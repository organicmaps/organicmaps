#include "features_vector.hpp"
#include "features_offsets_table.hpp"
#include "data_factory.hpp"
#include "dat_section_header.hpp"

#include "platform/constants.hpp"


FeaturesVector::FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                               feature::FeaturesOffsetsTable const * table,
                               indexer::MetadataDeserializer * metaDeserializer)
: m_loadInfo(cont, header), m_table(table), m_metaDeserializer(metaDeserializer)
{
  InitRecordsReader();
}

void FeaturesVector::InitRecordsReader()
{
  FilesContainerR::TReader reader = m_loadInfo.GetDataReader();

  feature::DatSectionHeader header;
  header.Read(*reader.GetPtr());
  CHECK(header.m_version == feature::DatSectionHeader::Version::V0,
          (base::Underlying(header.m_version)));
  m_recordReader = std::make_unique<RecordReader>(
        reader.SubReader(header.m_featuresOffset, header.m_featuresSize));
}

std::unique_ptr<FeatureType> FeaturesVector::GetByIndex(uint32_t index) const
{
  auto const ftOffset = m_table ? m_table->GetFeatureOffset(index) : index;
  return std::make_unique<FeatureType>(&m_loadInfo, m_recordReader->ReadRecord(ftOffset), m_metaDeserializer);
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
  : m_cont(cont), m_header(m_cont), m_vector(m_cont, m_header, nullptr, nullptr)
{
  m_vector.m_table = feature::FeaturesOffsetsTable::Load(m_cont).release();

  if (m_cont.IsExist(METADATA_FILE_TAG))
    m_vector.m_metaDeserializer = indexer::MetadataDeserializer::Load(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
  delete m_vector.m_metaDeserializer;
}
