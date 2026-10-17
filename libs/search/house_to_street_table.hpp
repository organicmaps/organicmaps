#pragma once
#include "indexer/house_to_street_iface.hpp"

#include "coding/map_uint32_to_val.hpp"

#include <memory>

class MwmValue;
class Writer;

namespace search
{
std::unique_ptr<HouseToStreetTable> LoadHouseToStreetTable(MwmValue const & value);
std::unique_ptr<HouseToStreetTable> LoadHouseToPlaceTable(MwmValue const & value);

class HouseToStreetTableBuilder
{
public:
  void Put(uint32_t houseId, uint32_t streetId);
  // Keys and values are stored relative to their table minimums.
  void Freeze(Writer & writer) const;

private:
  MapUint32ToValueBuilder<uint32_t> m_builder;
  bool m_empty = true;
  uint32_t m_keyOffset = 0;
  uint32_t m_valueOffset = 0;
};
}  // namespace search
