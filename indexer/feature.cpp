#include "indexer/feature.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"
#include "indexer/shared_load_info.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/parametrized_segment.hpp"
#include "geometry/robust_orientation.hpp"

#include "coding/byte_stream.hpp"
#include "coding/dd_vector.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/range_iterator.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <exception>
#include <limits>

#include "defines.hpp"

using namespace feature;
using namespace std;

namespace
{
uint32_t constexpr kInvalidOffset = numeric_limits<uint32_t>::max();

// Get the index for geometry serialization.
// @param[in]  scale:
// -1 : index for the best geometry
// -2 : index for the worst geometry
// default : needed geometry
int GetScaleIndex(SharedLoadInfo const & loadInfo, int scale)
{
  int const count = loadInfo.GetScalesCount();

  // In case of WorldCoasts we should get correct last geometry.
  int const lastScale = loadInfo.GetLastScale();
  if (scale > lastScale)
    scale = lastScale;

  switch (scale)
  {
  case FeatureType::WORST_GEOMETRY: return 0;
  case FeatureType::BEST_GEOMETRY: return count - 1;
  default:
    for (int i = 0; i < count; ++i)
    {
      if (scale <= loadInfo.GetScale(i))
        return i;
    }
    return -1;
  }
}

int GetScaleIndex(SharedLoadInfo const & loadInfo, int scale,
                  FeatureType::GeometryOffsets const & offsets)
{
  int ind = -1;
  int const count = static_cast<int>(offsets.size());

  // In case of WorldCoasts we should get correct last geometry.
  int const lastScale = loadInfo.GetLastScale();
  if (scale > lastScale)
    scale = lastScale;

  switch (scale)
  {
  case FeatureType::BEST_GEOMETRY:
    // Choose the best existing geometry for the last visible scale.
    ind = count - 1;
    while (ind >= 0 && offsets[ind] == kInvalidOffset)
      --ind;
    break;

  case FeatureType::WORST_GEOMETRY:
    // Choose the worst existing geometry for the first visible scale.
    ind = 0;
    while (ind < count && offsets[ind] == kInvalidOffset)
      ++ind;
    break;

  default:
  {
    int const n = loadInfo.GetScalesCount();
    for (int i = 0; i < n; ++i)
    {
      if (scale <= loadInfo.GetScale(i))
        return (offsets[i] != kInvalidOffset ? i : -1);
    }
  }
  }

  if (ind >= 0 && ind < count)
    return ind;

  ASSERT(false, ("Feature should have any geometry ..."));
  return -1;
}

uint32_t CalcOffset(ArrayByteSource const & source, FeatureType::Buffer const data)
{
  ASSERT_GREATER_OR_EQUAL(source.PtrC(), data, ());
  return static_cast<uint32_t>(source.PtrC() - data);
}

uint8_t Header(FeatureType::Buffer const data) { return static_cast<uint8_t>(*data); }

void ReadOffsets(SharedLoadInfo const & loadInfo, ArrayByteSource & src, uint8_t mask,
                 FeatureType::GeometryOffsets & offsets)
{
  ASSERT(offsets.empty(), ());
  ASSERT_GREATER(mask, 0, ());

  offsets.resize(loadInfo.GetScalesCount(), kInvalidOffset);
  size_t ind = 0;

  while (mask > 0)
  {
    if (mask & 0x01)
      offsets[ind] = ReadVarUint<uint32_t>(src);

    ++ind;
    mask = mask >> 1;
  }
}

class BitSource
{
  char const * m_ptr;
  uint8_t m_pos;

public:
  BitSource(char const * p) : m_ptr(p), m_pos(0) {}

  uint8_t Read(uint8_t count)
  {
    ASSERT_LESS(count, 9, ());

    uint8_t v = *m_ptr;
    v >>= m_pos;
    v &= ((1 << count) - 1);

    m_pos += count;
    if (m_pos >= 8)
    {
      ASSERT_EQUAL(m_pos, 8, ());
      ++m_ptr;
      m_pos = 0;
    }

    return v;
  }

  char const * RoundPtr()
  {
    if (m_pos > 0)
    {
      ++m_ptr;
      m_pos = 0;
    }
    return m_ptr;
  }
};

template <class TSource>
uint8_t ReadByte(TSource & src)
{
  return ReadPrimitiveFromSource<uint8_t>(src);
}
}  // namespace

void FeatureType::Deserialize(SharedLoadInfo const * loadInfo, Buffer buffer)
{
  CHECK(loadInfo, ());
  m_loadInfo = loadInfo;
  m_data = buffer;
  m_header = Header(m_data);

  m_offsets.Reset();
  m_ptsSimpMask = 0;
  m_limitRect = m2::RectD::GetEmptyRect();
  m_parsed.Reset();
  m_innerStats.MakeZero();
}

feature::EGeomType FeatureType::GetFeatureType() const
{
  switch (m_header & HEADER_GEOTYPE_MASK)
  {
  case HEADER_GEOM_LINE: return GEOM_LINE;
  case HEADER_GEOM_AREA: return GEOM_AREA;
  default: return GEOM_POINT;
  }
}

void FeatureType::SetTypes(array<uint32_t, feature::kMaxTypesCount> const & types, uint32_t count)
{
  ASSERT_GREATER_OR_EQUAL(count, 1, ("Must be one type at least."));
  ASSERT_LESS(count, feature::kMaxTypesCount, ("Too many types for feature"));
  count = min(count, static_cast<uint32_t>(feature::kMaxTypesCount - 1));
  m_types.fill(0);
  copy_n(begin(types), count, begin(m_types));
  auto value = static_cast<uint8_t>((count - 1) & feature::HEADER_TYPE_MASK);
  m_header = (m_header & (~feature::HEADER_TYPE_MASK)) | value;
}

void FeatureType::ParseTypes()
{
  if (m_parsed.m_types)
    return;

  auto const typesOffset = sizeof(m_header);
  Classificator & c = classif();
  ArrayByteSource source(m_data + typesOffset);

  size_t const count = GetTypesCount();
  uint32_t index = 0;
  try
  {
    for (size_t i = 0; i < count; ++i)
    {
      index = ReadVarUint<uint32_t>(source);
      m_types[i] = c.GetTypeForIndex(index);
    }
  }
  catch (std::out_of_range const & ex)
  {
    LOG(LERROR, ("Incorrect type index for feature.FeatureID:", m_id, ". Incorrect index:", index,
                 ". Loaded feature types:", m_types, ". Total count of types:", count,
                 ". Header:", m_header));
    throw;
  }

  m_offsets.m_common = CalcOffset(source, m_data);
  m_parsed.m_types = true;
}

void FeatureType::ParseCommon()
{
  if (m_parsed.m_common)
    return;

  CHECK(m_loadInfo, ());
  ParseTypes();

  ArrayByteSource source(m_data + m_offsets.m_common);
  uint8_t const h = Header(m_data);
  m_params.Read(source, h);

  if (GetFeatureType() == GEOM_POINT)
  {
    m_center = serial::LoadPoint(source, m_loadInfo->GetDefGeometryCodingParams());
    m_limitRect.Add(m_center);
  }

  m_offsets.m_header2 = CalcOffset(source, m_data);
  m_parsed.m_common = true;
}

m2::PointD FeatureType::GetCenter()
{
  ASSERT_EQUAL(GetFeatureType(), feature::GEOM_POINT, ());
  ParseCommon();
  return m_center;
}

int8_t FeatureType::GetLayer()
{
  if ((m_header & feature::HEADER_HAS_LAYER) == 0)
    return 0;

  ParseCommon();
  return m_params.layer;
}

void FeatureType::ReplaceBy(osm::EditableMapObject const & emo)
{
  uint8_t geoType;
  if (emo.IsPointType())
  {
    // We are here for existing point features and for newly created point features.
    m_center = emo.GetMercator();
    m_limitRect.MakeEmpty();
    m_limitRect.Add(m_center);
    m_parsed.m_points = m_parsed.m_triangles = true;
    geoType = feature::GEOM_POINT;
  }
  else
  {
    geoType = m_header & HEADER_GEOTYPE_MASK;
  }

  m_params.name = emo.GetName();
  string const & house = emo.GetHouseNumber();
  if (house.empty())
    m_params.house.Clear();
  else
    m_params.house.Set(house);
  m_parsed.m_common = true;

  m_metadata = emo.GetMetadata();
  m_parsed.m_metadata = true;

  uint32_t typesCount = 0;
  for (uint32_t const type : emo.GetTypes())
    m_types[typesCount++] = type;
  m_parsed.m_types = true;
  m_header = CalculateHeader(typesCount, geoType, m_params);
  m_parsed.m_header2 = true;

  m_id = emo.GetID();
}

void FeatureType::ParseEverything()
{
  // Also calls ParseCommon() and ParseTypes().
  ParseHeader2();
  ParseGeometry(FeatureType::BEST_GEOMETRY);
  ParseTriangles(FeatureType::BEST_GEOMETRY);
  ParseMetadata();
}

void FeatureType::ParseHeader2()
{
  if (m_parsed.m_header2)
    return;

  CHECK(m_loadInfo, ());
  ParseCommon();

  uint8_t ptsCount = 0, ptsMask = 0, trgCount = 0, trgMask = 0;
  BitSource bitSource(m_data + m_offsets.m_header2);
  uint8_t const typeMask = Header(m_data) & HEADER_GEOTYPE_MASK;

  if (typeMask == HEADER_GEOM_LINE)
  {
    ptsCount = bitSource.Read(4);
    if (ptsCount == 0)
      ptsMask = bitSource.Read(4);
    else
      ASSERT_GREATER(ptsCount, 1, ());
  }
  else if (typeMask == HEADER_GEOM_AREA)
  {
    trgCount = bitSource.Read(4);
    if (trgCount == 0)
      trgMask = bitSource.Read(4);
  }

  ArrayByteSource src(bitSource.RoundPtr());
  serial::GeometryCodingParams const & cp = m_loadInfo->GetDefGeometryCodingParams();

  if (typeMask == HEADER_GEOM_LINE)
  {
    if (ptsCount > 0)
    {
      int const count = ((ptsCount - 2) + 4 - 1) / 4;
      ASSERT_LESS(count, 4, ());

      for (int i = 0; i < count; ++i)
      {
        uint32_t mask = ReadByte(src);
        m_ptsSimpMask += (mask << (i << 3));
      }

      char const * start = src.PtrC();
      src = ArrayByteSource(serial::LoadInnerPath(start, ptsCount, cp, m_points));
      m_innerStats.m_points = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
    {
      m_points.emplace_back(serial::LoadPoint(src, cp));
      ReadOffsets(*m_loadInfo, src, ptsMask, m_offsets.m_pts);
    }
  }
  else if (typeMask == HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.PtrC());
      src = ArrayByteSource(serial::LoadInnerTriangles(start, trgCount, cp, m_triangles));
      m_innerStats.m_strips = CalcOffset(src, start);
    }
    else
    {
      ReadOffsets(*m_loadInfo, src, trgMask, m_offsets.m_trg);
    }
  }
  m_innerStats.m_size = CalcOffset(src, m_data);
  m_parsed.m_header2 = true;
}

void FeatureType::ResetGeometry()
{
  m_points.clear();
  m_triangles.clear();

  if (GetFeatureType() != GEOM_POINT)
    m_limitRect = m2::RectD();

  m_parsed.m_header2 = m_parsed.m_points = m_parsed.m_triangles = false;
  m_offsets.m_pts.clear();
  m_offsets.m_trg.clear();
  m_ptsSimpMask = 0;
}

uint32_t FeatureType::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if (!m_parsed.m_points)
  {
    CHECK(m_loadInfo, ());
    ParseHeader2();

    if ((Header(m_data) & HEADER_GEOTYPE_MASK) == HEADER_GEOM_LINE)
    {
      size_t const count = m_points.size();
      if (count < 2)
      {
        ASSERT_EQUAL(count, 1, ());

        // outer geometry
        int const ind = GetScaleIndex(*m_loadInfo, scale, m_offsets.m_pts);
        if (ind != -1)
        {
          ReaderSource<FilesContainerR::TReader> src(m_loadInfo->GetGeometryReader(ind));
          src.Skip(m_offsets.m_pts[ind]);

          serial::GeometryCodingParams cp = m_loadInfo->GetGeometryCodingParams(ind);
          cp.SetBasePoint(m_points[0]);
          serial::LoadOuterPath(src, cp, m_points);

          sz = static_cast<uint32_t>(src.Pos() - m_offsets.m_pts[ind]);
        }
      }
      else
      {
        // filter inner geometry

        FeatureType::Points points;
        points.reserve(count);

        int const scaleIndex = GetScaleIndex(*m_loadInfo, scale);
        ASSERT_LESS(scaleIndex, m_loadInfo->GetScalesCount(), ());

        points.emplace_back(m_points.front());
        for (size_t i = 1; i + 1 < count; ++i)
        {
          // check for point visibility in needed scaleIndex
          if (static_cast<int>((m_ptsSimpMask >> (2 * (i - 1))) & 0x3) <= scaleIndex)
            points.emplace_back(m_points[i]);
        }
        points.emplace_back(m_points.back());

        m_points.swap(points);
      }

      CalcRect(m_points, m_limitRect);
    }
    m_parsed.m_points = true;
  }
  return sz;
}

uint32_t FeatureType::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if (!m_parsed.m_triangles)
  {
    CHECK(m_loadInfo, ());
    ParseHeader2();

    if ((Header(m_data) & HEADER_GEOTYPE_MASK) == HEADER_GEOM_AREA)
    {
      if (m_triangles.empty())
      {
        auto const ind = GetScaleIndex(*m_loadInfo, scale, m_offsets.m_trg);
        if (ind != -1)
        {
          ReaderSource<FilesContainerR::TReader> src(m_loadInfo->GetTrianglesReader(ind));
          src.Skip(m_offsets.m_trg[ind]);
          serial::LoadOuterTriangles(src, m_loadInfo->GetGeometryCodingParams(ind), m_triangles);

          sz = static_cast<uint32_t>(src.Pos() - m_offsets.m_trg[ind]);
        }
      }

      CalcRect(m_triangles, m_limitRect);
    }
    m_parsed.m_triangles = true;
  }
  return sz;
}

void FeatureType::ParseMetadata()
{
  if (m_parsed.m_metadata)
    return;

  CHECK(m_loadInfo, ());
  try
  {
    struct TMetadataIndexEntry
    {
      uint32_t key;
      uint32_t value;
    };
    DDVector<TMetadataIndexEntry, FilesContainerR::TReader> idx(
        m_loadInfo->GetMetadataIndexReader());

    auto it = lower_bound(idx.begin(), idx.end(),
                          TMetadataIndexEntry{static_cast<uint32_t>(m_id.m_index), 0},
                          [](TMetadataIndexEntry const & v1, TMetadataIndexEntry const & v2) {
                            return v1.key < v2.key;
                          });

    if (it != idx.end() && m_id.m_index == it->key)
    {
      ReaderSource<FilesContainerR::TReader> src(m_loadInfo->GetMetadataReader());
      src.Skip(it->value);
      if (m_loadInfo->GetMWMFormat() >= version::Format::v8)
        m_metadata.Deserialize(src);
      else
        m_metadata.DeserializeFromMWMv7OrLower(src);
    }
  }
  catch (Reader::OpenException const &)
  {
    // now ignore exception because not all mwm have needed sections
  }

  m_parsed.m_metadata = true;
}

StringUtf8Multilang const & FeatureType::GetNames()
{
  ParseCommon();
  return m_params.name;
}

void FeatureType::SetNames(StringUtf8Multilang const & newNames)
{
  m_params.name.Clear();
  // Validate passed string to clean up empty names (if any).
  newNames.ForEach([this](int8_t langCode, string const & name) {
    if (!name.empty())
      m_params.name.AddString(langCode, name);
  });

  if (m_params.name.IsEmpty())
    m_header = m_header & ~feature::HEADER_HAS_NAME;
  else
    m_header = m_header | feature::HEADER_HAS_NAME;
}

void FeatureType::SetMetadata(feature::Metadata const & newMetadata)
{
  m_parsed.m_metadata = true;
  m_metadata = newMetadata;
}

void FeatureType::UpdateHeader(bool commonParsed, bool metadataParsed)
{
  feature::EHeaderTypeMask geomType =
      static_cast<feature::EHeaderTypeMask>(m_header & feature::HEADER_GEOTYPE_MASK);
  if (!geomType)
  {
    geomType = m_params.house.IsEmpty() && !m_params.ref.empty() ? feature::HEADER_GEOM_POINT
                                                                 : feature::HEADER_GEOM_POINT_EX;
  }

  m_header = feature::CalculateHeader(GetTypesCount(), geomType, m_params);
  m_parsed.m_header2 = true;
  m_parsed.m_types = true;

  m_parsed.m_common = commonParsed;
  m_parsed.m_metadata = metadataParsed;
}

bool FeatureType::UpdateMetadataValue(string const & key, string const & value)
{
  feature::Metadata::EType mdType;
  if (!feature::Metadata::TypeFromString(key, mdType))
    return false;
  m_metadata.Set(mdType, value);
  return true;
}

void FeatureType::ForEachMetadataItem(
    bool skipSponsored, function<void(string const & tag, string const & value)> const & fn) const
{
  for (auto const type : m_metadata.GetPresentTypes())
  {
    if (skipSponsored && m_metadata.IsSponsoredType(static_cast<feature::Metadata::EType>(type)))
      continue;
    auto const attributeName = ToString(static_cast<feature::Metadata::EType>(type));
    fn(attributeName, m_metadata.Get(type));
  }
}

void FeatureType::SetCenter(m2::PointD const & pt)
{
  ASSERT_EQUAL(GetFeatureType(), GEOM_POINT, ("Only for point feature."));
  m_center = pt;
  m_limitRect.Add(m_center);
  m_parsed.m_points = m_parsed.m_triangles = true;
}

namespace
{
  template <class TCont>
  void Points2String(string & s, TCont const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
      s += DebugPrint(points[i]) + " ";
  }
}

string FeatureType::DebugString(int scale)
{
  ParseCommon();

  Classificator const & c = classif();

  string res = "Types";
  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += (" : " + c.GetReadableObjectName(m_types[i]));
  res += "\n";

  res += m_params.DebugString();

  ParseGeometryAndTriangles(scale);
  switch (GetFeatureType())
  {
  case GEOM_POINT: res += (" Center:" + DebugPrint(m_center)); break;

  case GEOM_LINE:
    res += " Points:";
    Points2String(res, m_points);
    break;

  case GEOM_AREA:
    res += " Triangles:";
    Points2String(res, m_triangles);
    break;

  case GEOM_UNDEFINED:
    ASSERT(false, ("Assume that we have valid feature always"));
    break;
  }

  return res;
}

m2::RectD FeatureType::GetLimitRect(int scale)
{
  ParseGeometryAndTriangles(scale);

  if (m_triangles.empty() && m_points.empty() && (GetFeatureType() != GEOM_POINT))
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume that rect has zero dimensions.
    m_limitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_limitRect;
}

bool FeatureType::IsEmptyGeometry(int scale)
{
  ParseGeometryAndTriangles(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_triangles.empty();
  case GEOM_LINE: return m_points.empty();
  default: return false;
  }
}

size_t FeatureType::GetPointsCount() const
{
  ASSERT(m_parsed.m_points, ());
  return m_points.size();
}

m2::PointD const & FeatureType::GetPoint(size_t i) const
{
  ASSERT_LESS(i, m_points.size(), ());
  ASSERT(m_parsed.m_points, ());
  return m_points[i];
}

vector<m2::PointD> FeatureType::GetTriangesAsPoints(int scale)
{
  ParseTriangles(scale);
  return {m_triangles.begin(), m_triangles.end()};
}

void FeatureType::ParseGeometryAndTriangles(int scale)
{
  ParseGeometry(scale);
  ParseTriangles(scale);
}

FeatureType::GeomStat FeatureType::GetGeometrySize(int scale)
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_points.empty())
    sz = m_innerStats.m_points;

  return GeomStat(sz, m_points.size());
}

FeatureType::GeomStat FeatureType::GetTrianglesSize(int scale)
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_triangles.empty())
    sz = m_innerStats.m_strips;

  return GeomStat(sz, m_triangles.size());
}

void FeatureType::GetPreferredNames(string & primary, string & secondary)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  ::GetPreferredNames(mwmInfo->GetRegionData(), GetNames(), deviceLang, false /* allowTranslit */,
                      primary, secondary);
}

void FeatureType::GetPreferredNames(bool allowTranslit, int8_t deviceLang, string & primary,
                                    string & secondary)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  ::GetPreferredNames(mwmInfo->GetRegionData(), GetNames(), deviceLang, allowTranslit,
                      primary, secondary);
}

void FeatureType::GetReadableName(string & name)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  ::GetReadableName(mwmInfo->GetRegionData(), GetNames(), deviceLang, false /* allowTranslit */,
                    name);
}

void FeatureType::GetReadableName(bool allowTranslit, int8_t deviceLang, string & name)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  ::GetReadableName(mwmInfo->GetRegionData(), GetNames(), deviceLang, allowTranslit, name);
}

string FeatureType::GetHouseNumber()
{
  ParseCommon();
  return m_params.house.Get();
}

void FeatureType::SetHouseNumber(string const & number)
{
  if (number.empty())
    m_params.house.Clear();
  else
    m_params.house.Set(number);
}

bool FeatureType::GetName(int8_t lang, string & name)
{
  if (!HasName())
    return false;

  ParseCommon();
  return m_params.name.GetString(lang, name);
}

uint8_t FeatureType::GetRank()
{
  ParseCommon();
  return m_params.rank;
}

uint64_t FeatureType::GetPopulation() { return feature::RankToPopulation(GetRank()); }

string FeatureType::GetRoadNumber()
{
  ParseCommon();
  return m_params.ref;
}

feature::Metadata & FeatureType::GetMetadata()
{
  ParseMetadata();
  return m_metadata;
}
