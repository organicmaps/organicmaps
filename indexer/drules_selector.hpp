#pragma once

#include "indexer/feature.hpp"

#include <memory>
#include <string>
#include <vector>

namespace drule
{

// Runtime feature style selector absract interface.
class ISelector
{
public:
  virtual ~ISelector() = default;

  // If ISelector.Test returns true then style is applicable for the feature,
  // otherwise, if ISelector.Test returns false, style cannot be applied to the feature.
  virtual bool Test(FeatureType & ft, int zoom) const = 0;
};

// Factory method which builds ISelector from a string.
std::unique_ptr<ISelector> ParseSelector(std::string const & str);

// Factory method which builds composite ISelector from a set of string.
std::unique_ptr<ISelector> ParseSelector(std::vector<std::string> const & strs);

}  // namespace drule
