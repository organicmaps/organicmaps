#include "features_vector.hpp"
#include "features_offsets_table.hpp"
#include "mwm_version.hpp"
#include "data_factory.hpp"


void FeaturesVector::GetByIndex(uint32_t ind, FeatureType & ft) const
{
  uint32_t offset = 0, size = 0;
  m_RecordReader.ReadRecord(m_table ? m_table->GetFeatureOffset(ind) : ind, m_buffer, offset, size);
  ft.Deserialize(m_LoadInfo.GetLoader(), &m_buffer[offset]);
}


FeaturesVectorTest::FeaturesVectorTest(string const & filePath)
  : m_cont(filePath), m_initializer(this), m_vector(m_cont, m_header, 0)
{
  Init();
}

FeaturesVectorTest::FeaturesVectorTest(FilesContainerR const & cont)
  : m_cont(cont), m_initializer(this), m_vector(m_cont, m_header, 0)
{
  Init();
}

FeaturesVectorTest::Initializer::Initializer(FeaturesVectorTest * p)
{
  LoadMapHeader(p->m_cont, p->m_header);
}

void FeaturesVectorTest::Init()
{
  if (m_header.GetFormat() >= version::v5)
    m_vector.m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(m_cont).release();
}

FeaturesVectorTest::~FeaturesVectorTest()
{
  delete m_vector.m_table;
}
