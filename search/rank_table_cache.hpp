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

  struct Key : public MwmSet::MwmHandle
  {
    Key() = default;
    Key(Key &&) = default;

    explicit Key(Id const & id) { this->m_mwmId = id; }
    explicit Key(MwmSet::MwmHandle && handle) : MwmSet::MwmHandle(std::move(handle)) {}
  };

public:
  RankTableCache() = default;

  RankTable const & Get(DataSource & dataSource, Id const & mwmId);

  void Remove(Id const & id);
  void Clear();

private:
  struct Compare
  {
    bool operator()(Key const & r1, Key const & r2) const { return (r1.GetId() < r2.GetId()); }
  };

  std::map<Key, std::unique_ptr<RankTable>, Compare> m_ranks;

  DISALLOW_COPY_AND_MOVE(RankTableCache);
};
}  // namespace search
