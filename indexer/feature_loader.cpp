#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_loader.hpp"
#include "indexer/geometry_serialization.hpp"
#include "indexer/scales.hpp"

#include "geometry/pointu_to_uint64.hpp"

#include "coding/byte_stream.hpp"
#include "coding/dd_vector.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"

#include "defines.hpp"

namespace feature
{

uint8_t LoaderCurrent::GetHeader()
{
  return Header();
}

void LoaderCurrent::ParseTypes()
{
  Classificator & c = classif();

  ArrayByteSource source(DataPtr() + m_TypesOffset);

  size_t const count = m_pF->GetTypesCount();
  for (size_t i = 0; i < count; ++i)
    m_pF->m_types[i] = c.GetTypeForIndex(ReadVarUint<uint32_t>(source));

  m_CommonOffset = CalcOffset(source);
}

void LoaderCurrent::ParseCommon()
{
  ArrayByteSource source(DataPtr() + m_CommonOffset);

  uint8_t const h = Header();
  m_pF->m_params.Read(source, h);

  if (m_pF->GetFeatureType() == GEOM_POINT)
  {
    m_pF->m_center = serial::LoadPoint(source, GetDefCodingParams());
    m_pF->m_limitRect.Add(m_pF->m_center);
  }

  m_Header2Offset = CalcOffset(source);
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

void LoaderCurrent::ParseHeader2()
{
  uint8_t ptsCount = 0, ptsMask = 0, trgCount = 0, trgMask = 0;

  BitSource bitSource(DataPtr() + m_Header2Offset);

  uint8_t const typeMask = Header() & HEADER_GEOTYPE_MASK;
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

  serial::CodingParams const & cp = GetDefCodingParams();

  if (typeMask == HEADER_GEOM_LINE)
  {
    if (ptsCount > 0)
    {
      int const count = (ptsCount - 2 + 3) / 4;
      ASSERT_LESS ( count, 4, () );

      for (int i = 0; i < count; ++i)
      {
        uint32_t mask = ReadByte(src);
        m_ptsSimpMask += (mask << (i << 3));
      }

      char const * start = src.PtrC();

      src = ArrayByteSource(serial::LoadInnerPath(start, ptsCount, cp, m_pF->m_points));

      m_pF->m_innerStats.m_points = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
    {
      m_pF->m_points.push_back(serial::LoadPoint(src, cp));

      ReadOffsets(src, ptsMask, m_ptsOffsets);
    }
  }
  else if (typeMask == HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.PtrC());
      src = ArrayByteSource(serial::LoadInnerTriangles(start, trgCount, cp, m_pF->m_triangles));
      m_pF->m_innerStats.m_strips = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
    {
      ReadOffsets(src, trgMask, m_trgOffsets);
    }
  }

  m_pF->m_innerStats.m_size = static_cast<uint32_t>(src.PtrC() - DataPtr());
}

uint32_t LoaderCurrent::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if ((Header() & HEADER_GEOTYPE_MASK) == HEADER_GEOM_LINE)
  {
    size_t const count = m_pF->m_points.size();
    if (count < 2)
    {
      ASSERT_EQUAL ( count, 1, () );

      // outer geometry
      int const ind = GetScaleIndex(scale, m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::TReader> src(m_Info.GetGeometryReader(ind));
        src.Skip(m_ptsOffsets[ind]);

        serial::CodingParams cp = GetCodingParams(ind);
        cp.SetBasePoint(m_pF->m_points[0]);
        serial::LoadOuterPath(src, cp, m_pF->m_points);

        sz = static_cast<uint32_t>(src.Pos() - m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      FeatureType::points_t points;
      points.reserve(count);

      int const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_Info.GetScalesCount(), () );

      points.push_back(m_pF->m_points.front());
      for (size_t i = 1; i + 1 < count; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (static_cast<int>((m_ptsSimpMask >> (2 * (i - 1))) & 0x3) <= scaleIndex)
          points.push_back(m_pF->m_points[i]);
      }
      points.push_back(m_pF->m_points.back());

      m_pF->m_points.swap(points);
    }

    CalcRect(m_pF->m_points, m_pF->m_limitRect);
  }

  return sz;
}

uint32_t LoaderCurrent::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if ((Header() & HEADER_GEOTYPE_MASK) == HEADER_GEOM_AREA)
  {
    if (m_pF->m_triangles.empty())
    {
      auto const ind = GetScaleIndex(scale, m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::TReader> src(m_Info.GetTrianglesReader(ind));
        src.Skip(m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, GetCodingParams(ind), m_pF->m_triangles);

        sz = static_cast<uint32_t>(src.Pos() - m_trgOffsets[ind]);
      }
    }

    CalcRect(m_pF->m_triangles, m_pF->m_limitRect);
  }

  return sz;
}

void LoaderCurrent::ParseMetadata()
{
  try
  {
    struct TMetadataIndexEntry
    {
      uint32_t key;
      uint32_t value;
    };
    DDVector<TMetadataIndexEntry, FilesContainerR::TReader> idx(m_Info.GetMetadataIndexReader());

    auto it = lower_bound(
        idx.begin(), idx.end(),
        TMetadataIndexEntry{static_cast<uint32_t>(m_pF->m_id.m_index), 0},
        [](TMetadataIndexEntry const & v1, TMetadataIndexEntry const & v2)
        {
          return v1.key < v2.key;
        });

    if (it != idx.end() && m_pF->m_id.m_index == it->key)
    {
      ReaderSource<FilesContainerR::TReader> src(m_Info.GetMetadataReader());
      src.Skip(it->value);
      if (m_Info.GetMWMFormat() >= version::Format::v8)
        m_pF->m_metadata.Deserialize(src);
      else
        m_pF->m_metadata.DeserializeFromMWMv7OrLower(src);
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

int LoaderCurrent::GetScaleIndex(int scale, offsets_t const & offsets) const
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

}
