#pragma once

#include "indexer/data_factory.hpp"
#include "indexer/interval_index_iface.hpp"

#include "coding/var_serial_vector.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"


/// Index bucket <--> Draw scale range.
/// Using default one-to-one mapping.
class ScaleIndexBase
{
public:
  static uint32_t GetBucketsCount() { return 18; }
  static uint32_t BucketByScale(int scale) { return static_cast<uint32_t>(scale); }
  /// @return Range like [x, y).
  static pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket)
  {
    return {bucket, bucket + 1};
  }
};

template <class ReaderT>
class ScaleIndex : public ScaleIndexBase
{
public:
  typedef ReaderT ReaderType;

  ScaleIndex() = default;

  ScaleIndex(ReaderT const & reader, IndexFactory const & factory)
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

  void Attach(ReaderT const & reader, IndexFactory const & factory)
  {
    Clear();

    ReaderSource<ReaderT> source(reader);
    VarSerialVectorReader<ReaderT> treesReader(source);
    for (uint32_t i = 0; i < treesReader.Size(); ++i)
      m_IndexForScale.push_back(factory.CreateIndex(treesReader.SubReader(i)));
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, uint64_t beg, uint64_t end, int scale) const
  {
    auto const scaleBucket = BucketByScale(scale);
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
