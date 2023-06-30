#pragma once

#include "generator/sponsored_object_storage.hpp"
#include "generator/sponsored_scoring.hpp"

#include <functional>
#include <string>

namespace feature { class FeatureBuilder; }

namespace generator
{

template<typename SponsoredObject>
class SponsoredDataset
{
public:
  using Object = SponsoredObject;
  using ObjectId = typename Object::ObjectId;

  static double constexpr kDistanceLimitMeters = 150;
  static size_t constexpr kMaxSelectedElements = 3;

  explicit SponsoredDataset(std::string const & dataPath);

  /// @return true if |fb| satisfies some necessary conditions to match one or serveral objects from dataset.
  bool IsSponsoredCandidate(feature::FeatureBuilder const & fb) const;
  ObjectId FindMatchingObjectId(feature::FeatureBuilder const & fb) const;

  using FBuilderFnT = std::function<void(feature::FeatureBuilder &)>;
  // Applies changes to a given osm object (for example, remove hotel type)
  // and passes the result to |fn|.
  void PreprocessMatchedOsmObject(ObjectId matchedObjId, feature::FeatureBuilder & fb, FBuilderFnT const fn) const;
  // Creates objects and adds them to the map (MWM) via |fn|.
  void BuildOsmObjects(FBuilderFnT const & fn) const;

  static sponsored::MatchStats CalcScore(Object const & obj, feature::FeatureBuilder const & fb);
  sponsored::MatchStats CalcScore(ObjectId objId, feature::FeatureBuilder const & fb) const
  {
    return CalcScore(m_storage.GetObjectById(objId), fb);
  }

  SponsoredObjectStorage<Object> const & GetStorage() const { return m_storage; }

private:
  void BuildObject(Object const & object, FBuilderFnT const & fn) const;

  SponsoredObjectStorage<Object> m_storage;
};

}  // namespace generator
