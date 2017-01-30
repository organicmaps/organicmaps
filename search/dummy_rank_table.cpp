#include "search/dummy_rank_table.hpp"

#include "base/macros.hpp"

#include "std/limits.hpp"

namespace search
{
uint8_t DummyRankTable::Get(uint64_t /* i */) const { return 0; }

uint64_t DummyRankTable::Size() const
{
  NOTIMPLEMENTED();
  return numeric_limits<uint64_t>::max();
}

RankTable::Version DummyRankTable::GetVersion() const
{
  NOTIMPLEMENTED();
  return RankTable::VERSION_COUNT;
}

void DummyRankTable::Serialize(Writer & /* writer */, bool /* preserveHostEndianness */)
{
  NOTIMPLEMENTED();
}
}  // namespace search
