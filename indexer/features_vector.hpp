#pragma once
#include "../indexer/feature.hpp"

#include "../coding/var_record_reader.hpp"

#include "../base/base.hpp"

#include "../std/bind.hpp"


template <typename ReaderT>
class FeaturesVector
{
public:
  typedef ReaderT ReaderType;

  explicit FeaturesVector(ReaderT const & reader)
    : m_RecordReader(reader, 256), m_source(reader.GetName())
  {
  }

  void Get(uint64_t pos, FeatureType & feature) const
  {
    m_RecordReader.ReadRecord(pos, m_source.m_data, m_source.m_offset);
    feature.Deserialize(m_source);
  }

  template <class TDo> void ForEachOffset(TDo const & toDo) const
  {
    FeatureType f;
    m_RecordReader.ForEachRecord(
        bind<void>(toDo, bind(&FeaturesVector<ReaderT>::DeserializeFeature, this, _2, _3, &f), _1));
  }

  template <class TDo> void ForEach(TDo const & toDo) const
  {
    FeatureType f;
    m_RecordReader.ForEachRecord(
        bind<void>(toDo, bind(&FeaturesVector<ReaderT>::DeserializeFeature, this, _2, _3, &f)));
  }

  bool IsMyData(string const & fName) const
  {
    return m_RecordReader.IsEqual(fName);
  }

private:
  FeatureType const & DeserializeFeature(char const * data, uint32_t size, FeatureType * pFeature) const
  {
    m_source.assign(data, size);
    pFeature->Deserialize(m_source);
    return *pFeature;
  }

  VarRecordReader<ReaderT, &VarRecordSizeReaderVarint> m_RecordReader;
  mutable FeatureType::read_source_t m_source;
};
