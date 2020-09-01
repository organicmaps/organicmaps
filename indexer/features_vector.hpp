#pragma once

#include "indexer/dat_section_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/meta_idx.hpp"
#include "indexer/metadata_serdes.hpp"
#include "indexer/shared_load_info.hpp"

#include "coding/var_record_reader.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace feature { class FeaturesOffsetsTable; }

/// Note! This class is NOT Thread-Safe.
/// You should have separate instance of Vector for every thread.
class FeaturesVector
{
  DISALLOW_COPY(FeaturesVector);

public:
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header,
                 feature::FeaturesOffsetsTable const * table)
    : m_loadInfo(cont, header), m_table(table)
  {
    if (m_loadInfo.GetMWMFormat() >= version::Format::v11)
    {
      FilesContainerR::TReader reader = m_loadInfo.GetDataReader();

      feature::DatSectionHeader header;
      header.Read(*reader.GetPtr());
      CHECK(header.m_version == feature::DatSectionHeader::Version::V0,
            (base::Underlying(header.m_version)));
      m_recordReader = std::make_unique<RecordReader>(
          reader.SubReader(header.m_featuresOffset, header.m_featuresSize));

      auto metaReader = m_loadInfo.GetMetadataReader();
      m_metaDeserializer = indexer::MetadataDeserializer::Load(*metaReader.GetPtr());
      CHECK(m_metaDeserializer, ());
    }
    else if (m_loadInfo.GetMWMFormat() == version::Format::v10)
    {
      FilesContainerR::TReader reader = m_loadInfo.GetDataReader();

      feature::DatSectionHeader header;
      header.Read(*reader.GetPtr());
      CHECK(header.m_version == feature::DatSectionHeader::Version::V0,
            (base::Underlying(header.m_version)));
      m_recordReader = std::make_unique<RecordReader>(
          reader.SubReader(header.m_featuresOffset, header.m_featuresSize));

      auto metaIdxReader = m_loadInfo.GetMetadataIndexReader();
      m_metaidx = feature::MetadataIndex::Load(*metaIdxReader.GetPtr());
      CHECK(m_metaidx, ());
    }
    else
    {
      m_recordReader = std::make_unique<RecordReader>(m_loadInfo.GetDataReader());
    }
    CHECK(m_recordReader, ());
  }

  std::unique_ptr<FeatureType> GetByIndex(uint32_t index) const;

  size_t GetNumFeatures() const;

  template <class ToDo> void ForEach(ToDo && toDo) const
  {
    uint32_t index = 0;
    m_recordReader->ForEachRecord([&](uint32_t pos, std::vector<uint8_t> && data) {
      FeatureType ft(&m_loadInfo, std::move(data), m_metaidx.get(), m_metaDeserializer.get());

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
    RecordReader recordReader(reader);
    recordReader.ForEachRecord(
        [&](uint32_t pos, std::vector<uint8_t> && /* data */) { toDo(pos); });
  }

private:
  friend class FeaturesVectorTest;
  using RecordReader = VarRecordReader<FilesContainerR::TReader>;

  feature::SharedLoadInfo m_loadInfo;
  std::unique_ptr<RecordReader> m_recordReader;
  feature::FeaturesOffsetsTable const * m_table;
  std::unique_ptr<feature::MetadataIndex> m_metaidx;
  std::unique_ptr<indexer::MetadataDeserializer> m_metaDeserializer;
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
  explicit FeaturesVectorTest(std::string const & filePath);
  explicit FeaturesVectorTest(FilesContainerR const & cont);
  ~FeaturesVectorTest();

  feature::DataHeader const & GetHeader() const { return m_header; }
  FeaturesVector const & GetVector() const { return m_vector; }
};
