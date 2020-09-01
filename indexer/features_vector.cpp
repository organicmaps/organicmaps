#include "features_vector.hpp"
#include "features_offsets_table.hpp"
#include "data_factory.hpp"

#include "platform/constants.hpp"
#include "platform/mwm_version.hpp"

std::unique_ptr<FeatureType> FeaturesVector::GetByIndex(uint32_t index) const
{
  auto const ftOffset = m_table ? m_table->GetFeatureOffset(index) : index;
  return std::make_unique<FeatureType>(&m_loadInfo, m_recordReader->ReadRecord(ftOffset),
                                       m_metaidx.get(), m_metaDeserializer.get());
}

size_t FeaturesVector::GetNumFeatures() const
{
  return m_table ? m_table->size() : 0;
}

FeaturesVectorTest::FeaturesVectorTest(std::string const & filePath)
  : FeaturesVectorTest((FilesContainerR(filePath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT)))
{
}

FeaturesVectorTest::FeaturesVectorTest(FilesContainerR const & cont)
  : m_cont(cont), m_header(m_cont), m_vector(m_cont, m_header, 0)
{
  auto const version = m_header.GetFormat();
  CHECK_GREATER(version, version::Format::v5, ("Old maps should not be registered."));
  m_vector.m_table = feature::FeaturesOffsetsTable::Load(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
}
