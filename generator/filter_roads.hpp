#pragma once

#include "generator/filter_interface.hpp"

namespace generator
{
class FilterRoads : public FilterInterface
{
public:
  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(OsmElement const & element) const override;
};
}  // namespace generator
