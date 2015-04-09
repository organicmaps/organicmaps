#pragma once
#include "indexer/feature.hpp"
#include "indexer/feature_loader_base.hpp"

#include "coding/var_record_reader.hpp"


/// Note! This class is NOT Thread-Safe.
/// You should have separate instance of Vector for every thread.
class FeaturesVector
{
public:
  FeaturesVector(FilesContainerR const & cont, feature::DataHeader const & header)
    : m_LoadInfo(cont, header), m_RecordReader(m_LoadInfo.GetDataReader(), 256)
  {
  }

  void Get(uint64_t pos, FeatureType & ft) const
  {
    uint32_t offset = 0, size = 0;
    m_RecordReader.ReadRecord(pos, m_buffer, offset, size);

    ft.Deserialize(m_LoadInfo.GetLoader(), &m_buffer[offset]);
  }

  template <class ToDo> void ForEachOffset(ToDo toDo) const
  {
    m_RecordReader.ForEachRecord(DoGetFeatures<ToDo>(m_LoadInfo, toDo));
  }

private:
  template <class ToDo> class DoGetFeatures
  {
    feature::SharedLoadInfo const & m_loadInfo;
    ToDo & m_toDo;

  public:
    DoGetFeatures(feature::SharedLoadInfo const & loadInfo, ToDo & toDo)
      : m_loadInfo(loadInfo), m_toDo(toDo)
    {
    }

    void operator() (uint32_t pos, char const * data, uint32_t /*size*/) const
    {
      FeatureType ft;
      ft.Deserialize(m_loadInfo.GetLoader(), data);

      m_toDo(ft, pos);
    }
  };

private:
  feature::SharedLoadInfo m_LoadInfo;
  VarRecordReader<FilesContainerR::ReaderT, &VarRecordSizeReaderVarint> m_RecordReader;
  mutable vector<char> m_buffer;
};
