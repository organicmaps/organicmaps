#include "generator/feature_generator.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/data_header.hpp"
#include "geometry/mercator.hpp"

#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include <functional>
#include "std/target_os.hpp"
#include <unordered_map>

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeaturesCollector implementation
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace feature
{

FeaturesCollector::FeaturesCollector(std::string const & fName)
  : m_datFile(fName)
{
  CHECK_EQUAL(GetFileSize(m_datFile), 0, ());
}

FeaturesCollector::~FeaturesCollector()
{
  FlushBuffer();
  // Check file size
  (void)GetFileSize(m_datFile);
}

// static
uint32_t FeaturesCollector::GetFileSize(FileWriter const & f)
{
  // .dat file should be less than 4Gb
  uint64_t const pos = f.Pos();
  uint32_t const ret = static_cast<uint32_t>(pos);

  CHECK_EQUAL(static_cast<uint64_t>(ret), pos, ("Feature offset is out of 32bit boundary!"));
  return ret;
}

template <typename ValueT, size_t ValueSizeT = sizeof(ValueT) + 1>
pair<char[ValueSizeT], uint8_t> PackValue(ValueT v)
{
  static_assert(is_integral<ValueT>::value, "Non integral value");
  static_assert(is_unsigned<ValueT>::value, "Non unsigned value");

  pair<char[ValueSizeT], uint8_t> res;
  res.second = 0;
  while (v > 127)
  {
    res.first[res.second++] = static_cast<uint8_t>((v & 127) | 128);
    v >>= 7;
  }
  res.first[res.second++] = static_cast<uint8_t>(v);
  return res;
}

void FeaturesCollector::FlushBuffer()
{
  m_datFile.Write(m_writeBuffer, m_writePosition);
  m_writePosition = 0;
}

void FeaturesCollector::Flush()
{
  FlushBuffer();
  m_datFile.Flush();
}

void FeaturesCollector::Write(char const * src, size_t size)
{
  do
  {
    if (m_writePosition == sizeof(m_writeBuffer))
      FlushBuffer();
    size_t const part_size = min(size, sizeof(m_writeBuffer) - m_writePosition);
    memcpy(&m_writeBuffer[m_writePosition], src, part_size);
    m_writePosition += part_size;
    size -= part_size;
    src += part_size;
  } while (size > 0);
}

uint32_t FeaturesCollector::WriteFeatureBase(std::vector<char> const & bytes, FeatureBuilder1 const & fb)
{
  size_t const sz = bytes.size();
  CHECK(sz != 0, ("Empty feature not allowed here!"));

  auto const & packedSize = PackValue(sz);
  Write(packedSize.first, packedSize.second);
  Write(&bytes[0], sz);

  m_bounds.Add(fb.GetLimitRect());
  return m_featureID++;
}

uint32_t FeaturesCollector::operator()(FeatureBuilder1 const & fb)
{
  FeatureBuilder1::Buffer bytes;
  fb.Serialize(bytes);
  uint32_t const featureId = WriteFeatureBase(bytes, fb);
  CHECK_LESS(0, m_featureID, ());
  return featureId;
}

FeaturesAndRawGeometryCollector::FeaturesAndRawGeometryCollector(std::string const & featuresFileName,
                                                                 std::string const & rawGeometryFileName)
  : FeaturesCollector(featuresFileName), m_rawGeometryFileStream(rawGeometryFileName)
{
  CHECK_EQUAL(GetFileSize(m_rawGeometryFileStream), 0, ());
}

FeaturesAndRawGeometryCollector::~FeaturesAndRawGeometryCollector()
{
  uint64_t terminator = 0;
  m_rawGeometryFileStream.Write(&terminator, sizeof(terminator));
  LOG(LINFO, ("Write", m_rawGeometryCounter, "geometries into", m_rawGeometryFileStream.GetName()));
}

uint32_t FeaturesAndRawGeometryCollector::operator()(FeatureBuilder1 const & fb)
{
  uint32_t const featureId = FeaturesCollector::operator()(fb);
  FeatureBuilder1::Geometry const & geom = fb.GetGeometry();
  if (geom.empty())
    return featureId;

  ++m_rawGeometryCounter;

  uint64_t numGeometries = geom.size();
  m_rawGeometryFileStream.Write(&numGeometries, sizeof(numGeometries));
  for (FeatureBuilder1::PointSeq const & points : geom)
  {
    uint64_t numPoints = points.size();
    m_rawGeometryFileStream.Write(&numPoints, sizeof(numPoints));
    m_rawGeometryFileStream.Write(points.data(),
                                  sizeof(FeatureBuilder1::PointSeq::value_type) * points.size());
  }
  return featureId;
}
}
