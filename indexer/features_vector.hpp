#pragma once

#include "indexer/feature.hpp"
#include "indexer/shared_load_info.hpp"

#include "coding/var_record_reader.hpp"


namespace feature { class FeaturesOffsetsTable; }

/// Note! This class is NOT Thread-Safe.
/// You should have separate instance of Vector for every thread.
class FeaturesVector
{
  DISALLOW_COPY(FeaturesVector);

public:
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                 feature::FeaturesOffsetsTable const * table)
    : m_loadInfo(cont, header), m_recordReader(m_loadInfo.GetDataReader(), 256), m_table(table)
  {
  }

  void GetByIndex(uint32_t index, FeatureType & ft) const;

  size_t GetNumFeatures() const;

  template <class ToDo> void ForEach(ToDo && toDo) const
  {
    uint32_t index = 0;
    m_recordReader.ForEachRecord([&](uint32_t pos, char const * data, uint32_t /*size*/) {
      FeatureType ft;
      ft.Deserialize(&m_loadInfo, data);

      // We can't properly set MwmId here, because FeaturesVector
      // works with FileContainerR, not with MwmId/MwmHandle/MwmValue.
      // But it's OK to set at least feature's index, because it can
      // be used later for Metadata loading.
      ft.SetID(FeatureID(MwmSet::MwmId(), index));
      toDo(ft, m_table ? index++ : pos);
    });
  }

  template <class ToDo> static void ForEachOffset(ModelReaderPtr reader, ToDo && toDo)
  {
    VarRecordReader<ModelReaderPtr, &VarRecordSizeReaderVarint> recordReader(reader, 256);
    recordReader.ForEachRecord([&] (uint32_t pos, char const * /*data*/, uint32_t /*size*/)
    {
      toDo(pos);
    });
  }

private:
  friend class FeaturesVectorTest;

  feature::SharedLoadInfo m_loadInfo;
  VarRecordReader<FilesContainerR::TReader, &VarRecordSizeReaderVarint> m_recordReader;
  mutable vector<char> m_buffer;
  feature::FeaturesOffsetsTable const * m_table;
};

/// Test features vector (reader) that combines all the needed data for stand-alone work.
/// Used in generator_tool and unit tests.
class FeaturesVectorTest
{
  DISALLOW_COPY(FeaturesVectorTest);

  FilesContainerR m_cont;
  feature::DataHeader m_header;
  FeaturesVector m_vector;

public:
  explicit FeaturesVectorTest(string const & filePath);
  explicit FeaturesVectorTest(FilesContainerR const & cont);
  ~FeaturesVectorTest();

  feature::DataHeader const & GetHeader() const { return m_header; }
  FeaturesVector const & GetVector() const { return m_vector; }
};
