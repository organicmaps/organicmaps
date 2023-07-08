#pragma once
#include "indexer/house_to_street_iface.hpp"

#include "coding/map_uint32_to_val.hpp"

#include <memory>

class MwmValue;
class Writer;

namespace search
{
std::unique_ptr<HouseToStreetTable> LoadHouseToStreetTable(MwmValue const & value);

class HouseToStreetTableBuilder
{
public:
  void Put(uint32_t featureId, uint32_t offset);
  void Freeze(Writer & writer) const;

private:
  MapUint32ToValueBuilder<uint32_t> m_builder;
};
}  // namespace search
