#pragma once

#include "indexer/data_factory.hpp"
#include "indexer/interval_index_iface.hpp"

#include "coding/var_serial_vector.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"


/// Index bucket <--> Draw scale range.
class ScaleIndexBase
{
public:
  static uint32_t GetBucketsCount();
  static uint32_t BucketByScale(uint32_t scale);
  /// @return Range like [x, y).
  static pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket);
};

template <class ReaderT>
class ScaleIndex : public ScaleIndexBase
{
public:
  typedef ReaderT ReaderType;

  ScaleIndex() {}
  explicit ScaleIndex(ReaderT const & reader, IndexFactory & factory)
  {
    Attach(reader, factory);
  }
  ~ScaleIndex()
  {
    Clear();
  }

  void Clear()
  {
    for_each(m_IndexForScale.begin(), m_IndexForScale.end(), DeleteFunctor());
    m_IndexForScale.clear();
  }

  void Attach(ReaderT const & reader, IndexFactory & factory)
  {
    Clear();

    ReaderSource<ReaderT> source(reader);
    VarSerialVectorReader<ReaderT> treesReader(source);
    for (int i = 0; i < treesReader.Size(); ++i)
      m_IndexForScale.push_back(factory.CreateIndex(treesReader.SubReader(i)));
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, uint64_t beg, uint64_t end, uint32_t scale) const
  {
    size_t const scaleBucket = BucketByScale(scale);
    if (scaleBucket < m_IndexForScale.size())
    {
      IntervalIndexIFace::FunctionT f1(cref(f));
      for (size_t i = 0; i <= scaleBucket; ++i)
        m_IndexForScale[i]->DoForEach(f1, beg, end);
    }
  }

private:
  vector<IntervalIndexIFace *> m_IndexForScale;
};
