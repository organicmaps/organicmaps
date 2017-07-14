#pragma once

#include "indexer/feature_decl.hpp"

#include <memory>
#include <set>
#include <utility>

namespace df
{
struct CustomFeaturesContext
{
  std::set<FeatureID> const m_features;

  explicit CustomFeaturesContext(std::set<FeatureID> && features)
    : m_features(std::move(features))
  {}

  bool Contains(FeatureID const & id) const
  {
    return m_features.find(id) != m_features.cend();
  }
};

using CustomFeaturesContextPtr = std::shared_ptr<CustomFeaturesContext>;
using CustomFeaturesContextWeakPtr = std::weak_ptr<CustomFeaturesContext>;
} //  namespace df
