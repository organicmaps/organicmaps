#pragma once

#include "interval_index.hpp"
#include "../coding/var_serial_vector.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/macros.hpp"
#include "../std/algorithm.hpp"

class ScaleIndexBase
{
public:
  enum { NUM_BUCKETS = 18 };

  ScaleIndexBase()
  {
#ifdef DEBUG
    for (size_t i = 0; i < ARRAY_SIZE(kScaleBuckets); ++i)
    {
      ASSERT_LESS(kScaleBuckets[i], static_cast<uint32_t>(NUM_BUCKETS), (i));
      ASSERT(i == 0 || kScaleBuckets[i] >= kScaleBuckets[i-1],
             (i, kScaleBuckets[i-1], kScaleBuckets[i]));
    }
#endif
  }

  static uint32_t BucketByScale(uint32_t scale)
  {
    ASSERT_LESS(scale, ARRAY_SIZE(kScaleBuckets), ());
    return scale >= ARRAY_SIZE(kScaleBuckets) ? NUM_BUCKETS - 1 : kScaleBuckets[scale];
  }

  static pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket)
  {
    // TODO: Cache ScaleRangeForBucket in class member?
    ASSERT_LESS(bucket, static_cast<uint32_t>(NUM_BUCKETS), ());
    pair<uint32_t, uint32_t> res(ARRAY_SIZE(kScaleBuckets), 0);
    for (uint32_t i = 0; i < ARRAY_SIZE(kScaleBuckets); ++i)
    {
      if (kScaleBuckets[i] == bucket)
      {
        res.first = min(res.first, i);
        res.second = max(res.second, i + 1);
      }
    }
    return res;
  }

private:
  static uint32_t const kScaleBuckets[18];
};

template <class ReaderT>
class ScaleIndex : public ScaleIndexBase
{
public:
  typedef ReaderT ReaderType;
  typedef IntervalIndex<ReaderT> IntervalIndexType;
  typedef typename IntervalIndexType::Query Query;

  ScaleIndex() {}
  explicit ScaleIndex(ReaderT const & reader) { Attach(reader); }

  void Attach(ReaderT const & reader)
  {
    m_IndexForScale.clear();

    ReaderSource<ReaderT> source(reader);
    VarSerialVectorReader<ReaderT> treesReader(source);
    for (size_t i = 0; i < treesReader.Size(); ++i)
      m_IndexForScale.push_back(IntervalIndexType(treesReader.SubReader(i)));
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, uint64_t beg, uint64_t end, uint32_t scale,
                                 Query & query) const
  {
    size_t const scaleBucket = BucketByScale(scale);
    if (scaleBucket < m_IndexForScale.size())
      for (size_t i = 0; i <= scaleBucket; ++i)
        m_IndexForScale[i].ForEach(f, beg, end, query);
  }

private:
  vector<IntervalIndexType> m_IndexForScale;
};
