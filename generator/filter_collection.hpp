#pragma once

#include "generator/collection_base.hpp"
#include "generator/filter_interface.hpp"

#include <memory>

struct OsmElement;
namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
// This class allows you to work with a group of filters as with one.
class FilterCollection : public CollectionBase<std::shared_ptr<FilterInterface>>, public FilterInterface
{
public:
  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(feature::FeatureBuilder const & feature) override;
};
}  // namespace generator
