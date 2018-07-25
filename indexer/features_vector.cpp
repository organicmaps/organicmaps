#include "features_vector.hpp"
#include "features_offsets_table.hpp"
#include "data_factory.hpp"

#include "platform/constants.hpp"
#include "platform/mwm_version.hpp"


void FeaturesVector::GetByIndex(uint32_t index, FeatureType & ft) const
{
  uint32_t offset = 0, size = 0;
  auto const ftOffset = m_table ? m_table->GetFeatureOffset(index) : index;
  m_recordReader.ReadRecord(ftOffset, m_buffer, offset, size);
  ft.Deserialize(&m_loadInfo, &m_buffer[offset]);
}

size_t FeaturesVector::GetNumFeatures() const
{
  return m_table ? m_table->size() : 0;
}

FeaturesVectorTest::FeaturesVectorTest(string const & filePath)
  : FeaturesVectorTest((FilesContainerR(filePath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT)))
{
}

FeaturesVectorTest::FeaturesVectorTest(FilesContainerR const & cont)
  : m_cont(cont), m_header(m_cont), m_vector(m_cont, m_header, 0)
{
  auto const version = m_header.GetFormat();
  if (version == version::Format::v5)
    m_vector.m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(m_cont).release();
  else if (version >= version::Format::v6)
    m_vector.m_table = feature::FeaturesOffsetsTable::Load(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
}
