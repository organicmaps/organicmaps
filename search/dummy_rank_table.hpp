#pragma once

#include "indexer/rank_table.hpp"

#include <cstdint>

namespace search
{
// This dummy rank table is used instead of a normal rank table when
// the latter can't be loaded. It should not be serialized and can't
// be loaded.
class DummyRankTable : public RankTable
{
public:
  // RankTable overrides:
  uint8_t Get(uint64_t i) const override;
  uint64_t Size() const override;
  Version GetVersion() const override;
  void Serialize(Writer &) override;
};
}  // namespace search
