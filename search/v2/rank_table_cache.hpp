#pragma once

#include "indexer/mwm_set.hpp"

#include "std/map.hpp"
#include "std/unique_ptr.hpp"

#include "base/macros.hpp"

class MwmValue;

namespace search
{
class RankTable;

namespace v2
{
class RankTableCache
{
public:
  RankTableCache();

  ~RankTableCache();

  RankTable const & Get(MwmValue & value, MwmSet::MwmId const & mwmId);

  void Clear();

private:
  map<MwmSet::MwmId, unique_ptr<RankTable>> m_ranks;

  DISALLOW_COPY_AND_MOVE(RankTableCache);
};
}  // namespace v2
}  // namespace search
