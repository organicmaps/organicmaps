#pragma once

#include "indexer/mwm_set.hpp"

#include "base/macros.hpp"

#include <map>
#include <memory>
#include <utility>

class DataSource;

namespace search
{
class RankTable;

class RankTableCache
{
  using Id = MwmSet::MwmId;

  struct TKey : public MwmSet::MwmHandle
  {
    TKey() = default;
    TKey(TKey &&) = default;

    explicit TKey(Id const & id) { this->m_mwmId = id; }
    explicit TKey(MwmSet::MwmHandle && handle) : MwmSet::MwmHandle(std::move(handle)) {}
  };

public:
  RankTableCache() = default;

  RankTable const & Get(DataSource & dataSource, Id const & mwmId);

  void Remove(Id const & id);
  void Clear();

private:
  struct Compare
  {
    bool operator()(TKey const & r1, TKey const & r2) const { return (r1.GetId() < r2.GetId()); }
  };

  std::map<TKey, std::unique_ptr<RankTable>, Compare> m_ranks;

  DISALLOW_COPY_AND_MOVE(RankTableCache);
};
}  // namespace search
