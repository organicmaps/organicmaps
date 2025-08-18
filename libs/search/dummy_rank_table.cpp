#include "search/dummy_rank_table.hpp"

#include "base/macros.hpp"

namespace search
{
uint8_t DummyRankTable::Get(uint64_t /* i */) const
{
  return kNoRank;
}

uint64_t DummyRankTable::Size() const
{
  NOTIMPLEMENTED();
  return 0;
}

RankTable::Version DummyRankTable::GetVersion() const
{
  NOTIMPLEMENTED();
  return RankTable::VERSION_COUNT;
}

void DummyRankTable::Serialize(Writer &)
{
  NOTIMPLEMENTED();
}
}  // namespace search
