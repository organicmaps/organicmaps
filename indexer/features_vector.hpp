#pragma once
#include "feature.hpp"
#include "data_header_reader.hpp"

#include "../storage/defines.hpp"

#include "../coding/var_record_reader.hpp"

#include "../base/base.hpp"

//#include "../std/bind.hpp"


template <class ReaderT>
struct FeatureReaders
{
  ReaderT m_datR, m_geomR, m_trgR;

  template <class ContainerT>
  FeatureReaders(ContainerT const & cont)
    : m_datR(cont.GetReader(DATA_FILE_TAG)),
      m_geomR(cont.GetReader(GEOMETRY_FILE_TAG)),
      m_trgR(cont.GetReader(TRIANGLE_FILE_TAG))
  {
    uint64_t const offset = feature::GetSkipHeaderSize(m_datR);
    m_datR = m_datR.SubReader(offset, m_datR.Size() - offset);
  }
};

template <typename ReaderT>
class FeaturesVector
{
public:
  typedef ReaderT ReaderType;

  FeaturesVector(FeatureReaders<ReaderT> const & dataR)
    : m_RecordReader(dataR.m_datR, 256), m_source(dataR.m_geomR, dataR.m_trgR)
  {
  }

  void Get(uint64_t pos, FeatureType & feature) const
  {
    m_RecordReader.ReadRecord(pos, m_source.m_data, m_source.m_offset);
    feature.Deserialize(m_source);
  }

  template <class ToDo> void ForEachOffset(ToDo toDo) const
  {
    m_RecordReader.ForEachRecord(feature_getter<ToDo>(toDo, m_source));
  }

  //template <class TDo> void ForEach(TDo const & toDo) const
  //{
  //  FeatureType f;
  //  m_RecordReader.ForEachRecord(
  //      bind<void>(toDo, bind(&FeaturesVector<ReaderT>::DeserializeFeature, this, _2, _3, &f)));
  //}

  bool IsMyData(string const & fName) const
  {
    return m_RecordReader.IsEqual(fName);
  }

private:
  template <class ToDo> class feature_getter
  {
    ToDo & m_toDo;
    FeatureType::read_source_t & m_source;

  public:
    feature_getter(ToDo & toDo, FeatureType::read_source_t & src)
      : m_toDo(toDo), m_source(src)
    {
    }
    void operator() (uint32_t pos, char const * data, uint32_t size) const
    {
      FeatureType f;
      m_source.assign(data, size);
      f.Deserialize(m_source);
      m_toDo(f, pos);
    }
  };

  VarRecordReader<ReaderT, &VarRecordSizeReaderVarint> m_RecordReader;
  mutable FeatureType::read_source_t m_source;
};
