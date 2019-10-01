#pragma once

#include "generator/filter_interface.hpp"

#include <memory>

namespace generator
{
// The filter will leave only elements for complex(checks fb is attraction, eat or airport).
class FilterComplex : public FilterInterface
{
public:
  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(feature::FeatureBuilder const & fb) override;
};
}  // namespace generator
