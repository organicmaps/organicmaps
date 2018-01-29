#pragma once

#include "indexer/data_factory.hpp"
#include "indexer/interval_index.hpp"

#include "coding/var_serial_vector.hpp"

#include <cstdint>
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
  static std::pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket)
  {
    return {bucket, bucket + 1};
  }
};

template <class Reader>
class ScaleIndex : public ScaleIndexBase
{
public:
  ScaleIndex() = default;

  ScaleIndex(Reader const & reader, IndexFactory const & factory) { Attach(reader, factory); }

  ~ScaleIndex()
  {
    Clear();
  }

  void Clear()
  {
    m_IndexForScale.clear();
  }

  void Attach(Reader const & reader, IndexFactory const & factory)
  {
    Clear();

    ReaderSource<Reader> source(reader);
    VarSerialVectorReader<Reader> treesReader(source);
    for (uint32_t i = 0; i < treesReader.Size(); ++i)
      m_IndexForScale.push_back(factory.CreateIndex(treesReader.SubReader(i)));
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, uint64_t beg, uint64_t end, int scale) const
  {
    auto const scaleBucket = BucketByScale(scale);
    if (scaleBucket < m_IndexForScale.size())
    {
      for (size_t i = 0; i <= scaleBucket; ++i)
        m_IndexForScale[i]->ForEach(f, beg, end);
    }
  }

private:
  std::vector<std::unique_ptr<IntervalIndex<Reader, uint32_t>>> m_IndexForScale;
};
