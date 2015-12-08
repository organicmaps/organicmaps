#pragma once

#include "std/limits.hpp"
#include "std/unique_ptr.hpp"

class MwmValue;

namespace search
{
namespace v2
{
class HouseToStreetTable
{
public:
  virtual ~HouseToStreetTable() = default;

  static unique_ptr<HouseToStreetTable> Load(MwmValue & value);

  // Returns an order number for a street corresponding to |houseId|.
  // The order number is an index of a street in a list returned
  // by ReverseGeocoder::GetNearbyStreets().
  virtual uint32_t Get(uint32_t houseId) const = 0;
};
}  // namespace v2
}  // namespace search
