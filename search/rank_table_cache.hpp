#pragma once

#include "indexer/mwm_set.hpp"

#include "base/macros.hpp"

#include "std/map.hpp"
#include "std/unique_ptr.hpp"

class DataSource;

namespace search
{
class RankTable;

class RankTableCache
{
  using TId = MwmSet::MwmId;

  struct TKey : public MwmSet::MwmHandle
  {
    TKey() = default;
    TKey(TKey &&) = default;

    explicit TKey(TId const & id) { this->m_mwmId = id; }
    explicit TKey(MwmSet::MwmHandle && handle) : MwmSet::MwmHandle(move(handle)) {}
  };

public:
  RankTableCache() = default;

  RankTable const & Get(DataSource & dataSource, TId const & mwmId);

  void Remove(TId const & id);
  void Clear();

private:
  struct Compare
  {
    bool operator()(TKey const & r1, TKey const & r2) const { return (r1.GetId() < r2.GetId()); }
  };

  map<TKey, unique_ptr<RankTable>, Compare> m_ranks;

  DISALLOW_COPY_AND_MOVE(RankTableCache);
};

}  // namespace search
