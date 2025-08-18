#include "features_vector.hpp"
#include "dat_section_header.hpp"
#include "features_offsets_table.hpp"

#include "platform/constants.hpp"

FeaturesVector::FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                               feature::FeaturesOffsetsTable const * ftTable,
                               feature::FeaturesOffsetsTable const * relTable,
                               indexer::MetadataDeserializer * metaDeserializer)
  : m_loadInfo(cont, header, relTable, metaDeserializer)
  , m_table(ftTable)
{
  InitRecordsReader();
}

void FeaturesVector::InitRecordsReader()
{
  FilesContainerR::TReader reader = m_loadInfo.GetDataReader();
  ReaderSource src(reader);

  feature::DatSectionHeader header;
  header.Read(src);

  m_loadInfo.m_version = header.m_version;

  m_recordReader = std::make_unique<RecordReader>(reader.SubReader(header.m_featuresOffset, header.m_featuresSize));
}

std::unique_ptr<FeatureType> FeaturesVector::GetByIndex(uint32_t index) const
{
  auto const ftOffset = m_table ? m_table->GetFeatureOffset(index) : index;
  return std::make_unique<FeatureType>(&m_loadInfo, m_recordReader->ReadRecord(ftOffset));
}

size_t FeaturesVector::GetNumFeatures() const
{
  return m_table ? m_table->size() : 0;
}

FeaturesVectorTest::FeaturesVectorTest(std::string const & filePath)
  : FeaturesVectorTest((FilesContainerR(filePath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT)))
{}

FeaturesVectorTest::FeaturesVectorTest(FilesContainerR const & cont)
  : m_cont(cont)
  , m_header(m_cont)
  , m_vector(m_cont, m_header, nullptr, nullptr, nullptr)
{
  m_vector.m_table = feature::FeaturesOffsetsTable::Load(m_cont, FEATURE_OFFSETS_FILE_TAG).release();

  if (m_cont.IsExist(RELATION_OFFSETS_FILE_TAG))
    m_vector.m_loadInfo.m_relTable = feature::FeaturesOffsetsTable::Load(m_cont, RELATION_OFFSETS_FILE_TAG).release();

  if (m_cont.IsExist(METADATA_FILE_TAG))
    m_vector.m_loadInfo.m_metaDeserializer = indexer::MetadataDeserializer::Load(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
  delete m_vector.m_loadInfo.m_metaDeserializer;
  delete m_vector.m_loadInfo.m_relTable;
}
