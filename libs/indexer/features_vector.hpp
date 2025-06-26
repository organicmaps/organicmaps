#pragma once

#include "indexer/feature.hpp"
#include "indexer/shared_load_info.hpp"

#include "coding/var_record_reader.hpp"

#include <memory>
#include <vector>

namespace feature
{
class FeaturesOffsetsTable;
}

/// Note! This class is NOT Thread-Safe.
/// You should have separate instance of Vector for every thread.
class FeaturesVector
{
  DISALLOW_COPY(FeaturesVector);

public:
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                 feature::FeaturesOffsetsTable const * ftTable, feature::FeaturesOffsetsTable const * relTable,
                 indexer::MetadataDeserializer * metaDeserializer);

  std::unique_ptr<FeatureType> GetByIndex(uint32_t index) const;

  size_t GetNumFeatures() const;

  template <class ToDo>
  void ForEach(ToDo && toDo) const
  {
    uint32_t index = 0;
    m_recordReader->ForEachRecord([&](uint32_t pos, std::vector<uint8_t> && data)
    {
      FeatureType ft(&m_loadInfo, std::move(data));

      // We can't properly set MwmId here, because FeaturesVector
      // works with FileContainerR, not with MwmId/MwmHandle/MwmValue.
      // But it's OK to set at least feature's index, because it can
      // be used later for Metadata loading.
      ft.SetID(FeatureID(MwmSet::MwmId(), index));
      toDo(ft, m_table ? index++ : pos);
    });
  }

  template <class ToDo>
  static void ForEachOffset(FilesContainerR const & cont, ToDo && toDo)
  {
    feature::DataHeader header(cont);
    FeaturesVector vec(cont, header);
    vec.m_recordReader->ForEachRecord([&](uint32_t pos, std::vector<uint8_t> && /* data */) { toDo(pos); });
  }

private:
  /// Actually, this ctor is needed only for ForEachOffset call.
  /// Didn't find a better solution without big refactoring.
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header)
    : m_loadInfo(cont, header, nullptr, nullptr)
  {
    InitRecordsReader();
  }

  void InitRecordsReader();

  friend class FeaturesVectorTest;
  using RecordReader = VarRecordReader<FilesContainerR::TReader>;

  feature::SharedLoadInfo m_loadInfo;
  std::unique_ptr<RecordReader> m_recordReader;
  feature::FeaturesOffsetsTable const * m_table;
};

/// Test features vector (reader) that combines all the needed data for stand-alone work.
/// Used in generator_tool and unit tests.
class FeaturesVectorTest
{
  DISALLOW_COPY_AND_MOVE(FeaturesVectorTest);

  FilesContainerR m_cont;
  feature::DataHeader m_header;
  FeaturesVector m_vector;

public:
  explicit FeaturesVectorTest(std::string const & filePath);
  explicit FeaturesVectorTest(FilesContainerR const & cont);
  ~FeaturesVectorTest();

  FilesContainerR const & GetContainer() const { return m_cont; }
  feature::DataHeader const & GetHeader() const { return m_header; }
  FeaturesVector const & GetVector() const { return m_vector; }
};
