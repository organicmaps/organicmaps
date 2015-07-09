#include "generator/feature_generator.hpp"
#include "generator/data_cache_file.hpp"
#include "generator/osm_element.hpp"

#include "generator/osm_decl.hpp"
#include "generator/generate_info.hpp"

#include "indexer/data_header.hpp"
#include "indexer/mercator.hpp"
#include "indexer/cell_id.hpp"

#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/unordered_map.hpp"
#include "std/target_os.hpp"


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeaturesCollector implementation
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace feature {

FeaturesCollector::FeaturesCollector(string const & fName, string const &dumpFileName)
: m_datFile(fName)
, m_dumpFileName(dumpFileName)
{
  CHECK_EQUAL(GetFileSize(m_datFile), 0, ());
  if (!m_dumpFileName.empty())
  {
    m_dumpFileStream.open(m_dumpFileName.c_str(), ios::binary | ios::trunc | ios::out);
  }
}

FeaturesCollector::~FeaturesCollector()
{
  FlushBuffer();
  /// Check file size
  (void)GetFileSize(m_datFile);
  m_dumpFileStream.close();
}

uint32_t FeaturesCollector::GetFileSize(FileWriter const & f)
{
  // .dat file should be less than 4Gb
  uint64_t const pos = f.Pos();
  uint32_t const ret = static_cast<uint32_t>(pos);

  CHECK_EQUAL(static_cast<uint64_t>(ret), pos, ("Feature offset is out of 32bit boundary!"));
  return ret;
}

template <typename ValueT, size_t ValueSizeT = sizeof(ValueT)+1>
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
  m_baseOffset += m_writePosition;
  m_writePosition = 0;
}

void FeaturesCollector::Flush()
{
  FlushBuffer();
  m_datFile.Flush();
}

void FeaturesCollector::Write(char const *src, size_t size)
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
  } while(size > 0);
}


uint32_t FeaturesCollector::WriteFeatureBase(vector<char> const & bytes, FeatureBuilder1 const & fb)
{
  size_t const sz = bytes.size();
  CHECK(sz != 0, ("Empty feature not allowed here!"));

  size_t const offset = m_baseOffset + m_writePosition;

  auto const & packedSize = PackValue(sz);
  Write(packedSize.first, packedSize.second);
  Write(&bytes[0], sz);

  m_bounds.Add(fb.GetLimitRect());
  CHECK_EQUAL(offset, static_cast<uint32_t>(offset), ());
  return static_cast<uint32_t>(offset);
}

void FeaturesCollector::DumpFeatureGeometry(FeatureBuilder1 const & fb)
{

}

void FeaturesCollector::operator() (FeatureBuilder1 const & fb)
{
  FeatureBuilder1::buffer_t bytes;
  fb.Serialize(bytes);
  (void)WriteFeatureBase(bytes, fb);
}


}
