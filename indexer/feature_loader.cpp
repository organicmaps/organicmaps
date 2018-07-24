#include "indexer/feature_loader.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include "coding/byte_stream.hpp"
#include "coding/dd_vector.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/exception.hpp"
#include "std/limits.hpp"

#include "defines.hpp"

namespace feature
{
uint8_t LoaderCurrent::GetHeader(FeatureType const & ft) const { return Header(ft.m_data); }

void LoaderCurrent::ParseTypes(FeatureType & ft) const
{
  Classificator & c = classif();

  ArrayByteSource source(ft.m_data + ft.m_offsets.m_types);

  size_t const count = ft.GetTypesCount();
  uint32_t index = 0;
  try
  {
    for (size_t i = 0; i < count; ++i)
    {
      index = ReadVarUint<uint32_t>(source);
      ft.m_types[i] = c.GetTypeForIndex(index);
    }
  }
  catch (std::out_of_range const & ex)
  {
    LOG(LERROR, ("Incorrect type index for feature.FeatureID:", ft.m_id, ". Incorrect index:",
                 index, ". Loaded feature types:", ft.m_types, ". Total count of types:", count,
                 ". Header:", ft.m_header, ". Exception:", ex.what()));
    throw;
  }

  ft.m_offsets.m_common = CalcOffset(source, ft.m_data);
}

void LoaderCurrent::ParseCommon(FeatureType & ft) const
{
  ArrayByteSource source(ft.m_data + ft.m_offsets.m_common);

  uint8_t const h = Header(ft.m_data);
  ft.m_params.Read(source, h);

  if (ft.GetFeatureType() == GEOM_POINT)
  {
    ft.m_center = serial::LoadPoint(source, GetDefGeometryCodingParams());
    ft.m_limitRect.Add(ft.m_center);
  }

  ft.m_offsets.m_header2 = CalcOffset(source, ft.m_data);
}

namespace
{
  class BitSource
  {
    char const * m_ptr;
    uint8_t m_pos;

  public:
    BitSource(char const * p) : m_ptr(p), m_pos(0) {}

    uint8_t Read(uint8_t count)
    {
      ASSERT_LESS ( count, 9, () );

      uint8_t v = *m_ptr;
      v >>= m_pos;
      v &= ((1 << count) - 1);

      m_pos += count;
      if (m_pos >= 8)
      {
        ASSERT_EQUAL ( m_pos, 8, () );
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

  template <class TSource> uint8_t ReadByte(TSource & src)
  {
    return ReadPrimitiveFromSource<uint8_t>(src);
  }
}

void LoaderCurrent::ParseHeader2(FeatureType & ft) const
{
  uint8_t ptsCount = 0, ptsMask = 0, trgCount = 0, trgMask = 0;

  BitSource bitSource(ft.m_data + ft.m_offsets.m_header2);

  uint8_t const typeMask = Header(ft.m_data) & HEADER_GEOTYPE_MASK;
  if (typeMask == HEADER_GEOM_LINE)
  {
    ptsCount = bitSource.Read(4);
    if (ptsCount == 0)
      ptsMask = bitSource.Read(4);
    else
      ASSERT_GREATER ( ptsCount, 1, () );
  }
  else if (typeMask == HEADER_GEOM_AREA)
  {
    trgCount = bitSource.Read(4);
    if (trgCount == 0)
      trgMask = bitSource.Read(4);
  }

  ArrayByteSource src(bitSource.RoundPtr());

  serial::GeometryCodingParams const & cp = GetDefGeometryCodingParams();

  if (typeMask == HEADER_GEOM_LINE)
  {
    if (ptsCount > 0)
    {
      int const count = (ptsCount - 2 + 3) / 4;
      ASSERT_LESS ( count, 4, () );

      for (int i = 0; i < count; ++i)
      {
        uint32_t mask = ReadByte(src);
        ft.m_ptsSimpMask += (mask << (i << 3));
      }

      char const * start = src.PtrC();

      src = ArrayByteSource(serial::LoadInnerPath(start, ptsCount, cp, ft.m_points));

      ft.m_innerStats.m_points = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
    {
      ft.m_points.push_back(serial::LoadPoint(src, cp));

      ReadOffsets(src, ptsMask, ft.m_ptsOffsets);
    }
  }
  else if (typeMask == HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.PtrC());
      src = ArrayByteSource(serial::LoadInnerTriangles(start, trgCount, cp, ft.m_triangles));
      ft.m_innerStats.m_strips = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
    {
      ReadOffsets(src, trgMask, ft.m_trgOffsets);
    }
  }

  ft.m_innerStats.m_size = static_cast<uint32_t>(src.PtrC() - ft.m_data);
}

uint32_t LoaderCurrent::ParseGeometry(int scale, FeatureType & ft) const
{
  uint32_t sz = 0;
  if ((Header(ft.m_data) & HEADER_GEOTYPE_MASK) == HEADER_GEOM_LINE)
  {
    size_t const count = ft.m_points.size();
    if (count < 2)
    {
      ASSERT_EQUAL ( count, 1, () );

      // outer geometry
      int const ind = GetScaleIndex(scale, ft.m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::TReader> src(m_Info.GetGeometryReader(ind));
        src.Skip(ft.m_ptsOffsets[ind]);

        serial::GeometryCodingParams cp = GetGeometryCodingParams(ind);
        cp.SetBasePoint(ft.m_points[0]);
        serial::LoadOuterPath(src, cp, ft.m_points);

        sz = static_cast<uint32_t>(src.Pos() - ft.m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      FeatureType::Points points;
      points.reserve(count);

      int const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_Info.GetScalesCount(), () );

      points.push_back(ft.m_points.front());
      for (size_t i = 1; i + 1 < count; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (static_cast<int>((ft.m_ptsSimpMask >> (2 * (i - 1))) & 0x3) <= scaleIndex)
          points.push_back(ft.m_points[i]);
      }
      points.push_back(ft.m_points.back());

      ft.m_points.swap(points);
    }

    CalcRect(ft.m_points, ft.m_limitRect);
  }

  return sz;
}

uint32_t LoaderCurrent::ParseTriangles(int scale, FeatureType & ft) const
{
  uint32_t sz = 0;
  if ((Header(ft.m_data) & HEADER_GEOTYPE_MASK) == HEADER_GEOM_AREA)
  {
    if (ft.m_triangles.empty())
    {
      auto const ind = GetScaleIndex(scale, ft.m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::TReader> src(m_Info.GetTrianglesReader(ind));
        src.Skip(ft.m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, GetGeometryCodingParams(ind), ft.m_triangles);

        sz = static_cast<uint32_t>(src.Pos() - ft.m_trgOffsets[ind]);
      }
    }

    CalcRect(ft.m_triangles, ft.m_limitRect);
  }

  return sz;
}

void LoaderCurrent::ParseMetadata(FeatureType & ft) const
{
  try
  {
    struct TMetadataIndexEntry
    {
      uint32_t key;
      uint32_t value;
    };
    DDVector<TMetadataIndexEntry, FilesContainerR::TReader> idx(m_Info.GetMetadataIndexReader());

    auto it = lower_bound(idx.begin(), idx.end(),
                          TMetadataIndexEntry{static_cast<uint32_t>(ft.m_id.m_index), 0},
                          [](TMetadataIndexEntry const & v1, TMetadataIndexEntry const & v2) {
                            return v1.key < v2.key;
                          });

    if (it != idx.end() && ft.m_id.m_index == it->key)
    {
      ReaderSource<FilesContainerR::TReader> src(m_Info.GetMetadataReader());
      src.Skip(it->value);
      if (m_Info.GetMWMFormat() >= version::Format::v8)
        ft.m_metadata.Deserialize(src);
      else
        ft.m_metadata.DeserializeFromMWMv7OrLower(src);
    }
  }
  catch (Reader::OpenException const &)
  {
    // now ignore exception because not all mwm have needed sections
  }
}

int LoaderCurrent::GetScaleIndex(int scale) const
{
  int const count = m_Info.GetScalesCount();

  // In case of WorldCoasts we should get correct last geometry.
  int const lastScale = m_Info.GetLastScale();
  if (scale > lastScale)
    scale = lastScale;

  switch (scale)
  {
  case FeatureType::WORST_GEOMETRY: return 0;
  case FeatureType::BEST_GEOMETRY: return count - 1;
  default:
    for (int i = 0; i < count; ++i)
      if (scale <= m_Info.GetScale(i))
        return i;
    return -1;
  }
}

int LoaderCurrent::GetScaleIndex(int scale, FeatureType::GeometryOffsets const & offsets) const
{
  int ind = -1;
  int const count = static_cast<int>(offsets.size());

  // In case of WorldCoasts we should get correct last geometry.
  int const lastScale = m_Info.GetLastScale();
  if (scale > lastScale)
    scale = lastScale;

  switch (scale)
  {
  case FeatureType::BEST_GEOMETRY:
    // Choose the best existing geometry for the last visible scale.
    ind = count - 1;
    while (ind >= 0 && offsets[ind] == s_InvalidOffset) --ind;
    break;

  case FeatureType::WORST_GEOMETRY:
    // Choose the worst existing geometry for the first visible scale.
    ind = 0;
    while (ind < count && offsets[ind] == s_InvalidOffset) ++ind;
    break;

  default:
    {
      int const n = m_Info.GetScalesCount();
      for (int i = 0; i < n; ++i)
      {
        if (scale <= m_Info.GetScale(i))
          return (offsets[i] != s_InvalidOffset ? i : -1);
      }
    }
  }

  if (ind >= 0 && ind < count)
    return ind;
  else
  {
    ASSERT ( false, ("Feature should have any geometry ...") );
    return -1;
  }
}
}  // namespace feature
