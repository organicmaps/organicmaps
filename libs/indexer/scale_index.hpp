#pragma once

#include "indexer/interval_index.hpp"

#include "coding/var_serial_vector.hpp"

#include <memory>
#include <vector>

/// Index bucket <--> Draw scale range.
/// Using default one-to-one mapping.
class ScaleIndexBase
{
public:
  static uint32_t GetBucketsCount() { return 18; }
  static uint32_t BucketByScale(int scale) { return static_cast<uint32_t>(scale); }
  /// @return Range like [x, y).
  static std::pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket) { return {bucket, bucket + 1}; }
};

template <class Reader>
class ScaleIndex : public ScaleIndexBase
{
public:
  explicit ScaleIndex(Reader const & reader) { Attach(reader); }

  ~ScaleIndex() { Clear(); }

  void Clear() { m_IndexForScale.clear(); }

  void Attach(Reader const & reader)
  {
    Clear();

    ReaderSource<Reader> source(reader);
    VarSerialVectorReader<Reader> treesReader(source);
    for (uint32_t i = 0; i < treesReader.Size(); ++i)
      m_IndexForScale.push_back(std::make_unique<IndexT>(treesReader.SubReader(i)));
  }

  template <class FnT>
  void ForEachInIntervalAndScale(uint64_t beg, uint64_t end, int scale, FnT && fn) const
  {
    auto const scaleBucket = BucketByScale(scale);
    if (scaleBucket < m_IndexForScale.size())
      for (size_t i = 0; i <= scaleBucket; ++i)
        m_IndexForScale[i]->ForEach(fn, beg, end);
  }

private:
  using IndexT = IntervalIndex<Reader, uint32_t>;
  std::vector<std::unique_ptr<IndexT>> m_IndexForScale;
};
