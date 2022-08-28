#include "features_vector.hpp"
#include "features_offsets_table.hpp"
#include "data_factory.hpp"
#include "dat_section_header.hpp"

#include "platform/constants.hpp"


FeaturesVector::FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                               feature::FeaturesOffsetsTable const * table)
: m_loadInfo(cont, header), m_table(table)
{
  InitRecordsReader();

  /// @todo Probably, some kind of lazy m_metaDeserializer will be better here,
  /// depending on how often FeaturesVector ctor is called.
  auto metaReader = m_loadInfo.GetMetadataReader();
  m_metaDeserializer = indexer::MetadataDeserializer::Load(*metaReader.GetPtr());
  CHECK(m_metaDeserializer, ());
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
  return std::make_unique<FeatureType>(&m_loadInfo, m_recordReader->ReadRecord(ftOffset),
                                       m_metaDeserializer.get());
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
  : m_cont(cont), m_header(m_cont), m_vector(m_cont, m_header, nullptr)
{
  m_vector.m_table = feature::FeaturesOffsetsTable::Load(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
}
