#pragma once

#include "indexer/cell_id.hpp"

#include "std/vector.hpp"

namespace covering
{
class CellFeaturePair
{
public:
  CellFeaturePair() = default;
  CellFeaturePair(uint64_t cell, uint32_t feature)
    : m_CellLo(UINT64_LO(cell)), m_CellHi(UINT64_HI(cell)), m_Feature(feature)
  {
  }

  bool operator<(CellFeaturePair const & rhs) const
  {
    if (m_CellHi != rhs.m_CellHi)
      return m_CellHi < rhs.m_CellHi;
    if (m_CellLo != rhs.m_CellLo)
      return m_CellLo < rhs.m_CellLo;
    return m_Feature < rhs.m_Feature;
  }

  uint64_t GetCell() const { return UINT64_FROM_UINT32(m_CellHi, m_CellLo); }
  uint32_t GetFeature() const { return m_Feature; }
private:
  uint32_t m_CellLo;
  uint32_t m_CellHi;
  uint32_t m_Feature;
};
static_assert(sizeof(CellFeaturePair) == 12, "");
#ifndef OMIM_OS_LINUX
static_assert(is_trivially_copyable<CellFeaturePair>::value, "");
#endif

class CellFeatureBucketTuple
{
public:
  CellFeatureBucketTuple() = default;
  CellFeatureBucketTuple(CellFeaturePair const & p, uint32_t bucket) : m_pair(p), m_bucket(bucket)
  {
  }

  bool operator<(CellFeatureBucketTuple const & rhs) const
  {
    if (m_bucket != rhs.m_bucket)
      return m_bucket < rhs.m_bucket;
    return m_pair < rhs.m_pair;
  }

  CellFeaturePair const & GetCellFeaturePair() const { return m_pair; }
  uint32_t GetBucket() const { return m_bucket; }
private:
  CellFeaturePair m_pair;
  uint32_t m_bucket;
};
static_assert(sizeof(CellFeatureBucketTuple) == 16, "");
#ifndef OMIM_OS_LINUX
static_assert(is_trivially_copyable<CellFeatureBucketTuple>::value, "");
#endif

template <class TSorter>
class DisplacementManager
{
public:
  DisplacementManager(TSorter & sorter) : m_sorter(sorter) {}
  template <class TFeature>
  void Add(vector<int64_t> const & cells, uint32_t bucket, TFeature const & ft, uint32_t index)
  {
    if (!IsDisplacable(ft))
    {
      for (auto cell : cells)
        m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, index), bucket));
      return;
    }
    // TODO(ldragunov) Put feature to displacement store.
  }

private:
  // TODO write displacement determination.
  template <class TFeature>
  bool IsDisplacable(TFeature const & /*ft*/) const noexcept
  {
    return false;
  }

  TSorter & m_sorter;
};
}  // namespace indexer
