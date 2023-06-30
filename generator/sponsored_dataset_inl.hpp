#pragma once

#include "generator/feature_builder.hpp"
#include "generator/sponsored_dataset.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <string>


namespace generator
{

// SponsoredDataset --------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(std::string const & dataPath)
  : m_storage(kDistanceLimitMeters, kMaxSelectedElements)
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
sponsored::MatchStats SponsoredDataset<SponsoredObject>::CalcScore(
    Object const & obj, feature::FeatureBuilder const & fb)
{
  auto const fbCenter = mercator::ToLatLon(fb.GetKeyPoint());
  auto const distance = ms::DistanceOnEarth(fbCenter, obj.m_latLon);

  /// @todo Input dataset is in English language.
  auto name = fb.GetName(StringUtf8Multilang::kEnglishCode);
  if (name.empty())
    name = fb.GetName(StringUtf8Multilang::kDefaultCode);

  return { distance, kDistanceLimitMeters, obj.m_name, std::string(name) };
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::ObjectId
SponsoredDataset<SponsoredObject>::FindMatchingObjectId(feature::FeatureBuilder const & fb) const
{
  // Find |kMaxSelectedElements| nearest values to a point, sorted by distance?
  auto const indices = m_storage.GetNearestObjects(mercator::ToLatLon(fb.GetKeyPoint()));

  // Select best candidate by score.
  double bestScore = -1;
  auto res = Object::InvalidObjectId();
  for (auto const i : indices)
  {
    auto const r = CalcScore(i, fb);
    if (r.IsMatched())
    {
      double const score = r.GetMatchingScore();
      if (score > bestScore)
      {
        bestScore = score;
        res = i;
      }
    }
  }

  return res;
}

}  // namespace generator
