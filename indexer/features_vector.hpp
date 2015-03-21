#pragma once
#include "feature.hpp"
#include "feature_loader_base.hpp"
#include "features_offsets_table.hpp"

#include "platform/platform.hpp"

#include "coding/var_record_reader.hpp"
#include "coding/file_name_utils.hpp"


/// Note! This class is NOT Thread-Safe.
/// You should have separate instance of Vector for every thread.
class FeaturesVector
{
  unique_ptr<feature::FeaturesOffsetsTable> m_table;

public:
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header)
    : m_LoadInfo(cont, header), m_RecordReader(m_LoadInfo.GetDataReader(), 256)
  {
    if (header.GetFormat() >= version::v5)
    {
      string const & filePath = cont.GetFileName();
      size_t const sepIndex = filePath.rfind(my::GetNativeSeparator());
      string const name(filePath,  sepIndex + 1, filePath.rfind(DATA_FILE_EXTENSION) - sepIndex - 1);
      string const path = GetPlatform().GetIndexFileName(name, FEATURES_OFFSETS_TABLE_FILE_EXT);

      m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(path, cont);
    }
  }

  void GetByIndex(uint32_t ind, FeatureType & ft) const
  {
    uint32_t offset = 0, size = 0;
    m_RecordReader.ReadRecord(m_table ? m_table->GetFeatureOffset(ind) : ind, m_buffer, offset, size);
    ft.Deserialize(m_LoadInfo.GetLoader(), &m_buffer[offset]);
  }

  template <class ToDo> void ForEach(ToDo toDo) const
  {
    uint32_t ind = 0;
    m_RecordReader.ForEachRecord([&] (uint32_t pos, char const * data, uint32_t /*size*/)
    {
      FeatureType ft;
      ft.Deserialize(m_LoadInfo.GetLoader(), data);
      toDo(ft, m_table ? ind++ : pos);
    });
  }

  template <class ToDo> static void ForEachOffset(ModelReaderPtr reader, ToDo toDo)
  {
    VarRecordReader<ModelReaderPtr, &VarRecordSizeReaderVarint> recordReader(reader, 256);
    recordReader.ForEachRecord([&] (uint32_t pos, char const * /*data*/, uint32_t /*size*/)
    {
      toDo(pos);
    });
  }

private:
  feature::SharedLoadInfo m_LoadInfo;
  VarRecordReader<FilesContainerR::ReaderT, &VarRecordSizeReaderVarint> m_RecordReader;
  mutable vector<char> m_buffer;
};
