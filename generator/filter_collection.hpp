#pragma once

#include "generator/collection_base.hpp"
#include "generator/filter_interface.hpp"

#include <memory>

struct OsmElement;
class FeatureBuilder1;
namespace generator
{
// This class allows you to work with a group of filters as with one.
class FilterCollection : public CollectionBase<std::shared_ptr<FilterInterface>>, public FilterInterface
{
public:
  // FilterInterface overrides:
  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(FeatureBuilder1 const & feature) override;
};
}  // namespace generator
