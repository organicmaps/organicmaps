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

  explicit FeaturesVector(ReaderT const & reader) : m_RecordReader(reader, 256)
  {
  }

  void Get(uint64_t pos, FeatureType & feature) const
  {
    vector<char> record;
    uint32_t offset;
    m_RecordReader.ReadRecord(pos, record, offset);
    feature.Deserialize(record, offset);
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
    vector<char> data1(data, data + size);
    pFeature->Deserialize(data1);
    return *pFeature;
  }

  VarRecordReader<ReaderT, &VarRecordSizeReaderVarint> m_RecordReader;
};
