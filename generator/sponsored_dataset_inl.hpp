#pragma once

#include "generator/sponsored_dataset.hpp"

#include <memory>
#include <string>

namespace generator
{

// SponsoredDataset --------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(std::string const & dataPath)
  : m_storage(kDistanceLimitInMeters, kMaxSelectedElements)
{
  m_storage.LoadData(dataPath);
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::BuildOsmObjects(FBuilderFnT const & fn) const
{
  for (auto const & item : m_storage.GetObjects())
    BuildObject(item.second, fn);
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::ObjectId
SponsoredDataset<SponsoredObject>::FindMatchingObjectId(feature::FeatureBuilder const & fb) const
{
  if (NecessaryMatchingConditionHolds(fb))
    return FindMatchingObjectIdImpl(fb);
  return Object::InvalidObjectId();
}
}  // namespace generator
