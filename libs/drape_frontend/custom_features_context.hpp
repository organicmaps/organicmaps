#pragma once

#include "indexer/feature_decl.hpp"

#include <map>
#include <memory>
#include <utility>

namespace df
{
using CustomFeatures = std::map<FeatureID, bool>;

struct CustomFeaturesContext
{
  CustomFeatures const m_features;

  explicit CustomFeaturesContext(CustomFeatures && features) : m_features(std::move(features)) {}

  bool NeedDiscardGeometry(FeatureID const & id) const
  {
    auto const it = m_features.find(id);
    if (it == m_features.cend())
      return false;
    return it->second;
  }
};

using CustomFeaturesContextPtr = std::shared_ptr<CustomFeaturesContext>;
using CustomFeaturesContextWeakPtr = std::weak_ptr<CustomFeaturesContext>;
}  //  namespace df
