#pragma once

#include "generator/sponsored_object_storage.hpp"

#include "base/newtype.hpp"

#include <functional>
#include <string>

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
template<typename SponsoredObject>
class SponsoredDataset
{
public:
  using Object = SponsoredObject;
  using ObjectId = typename Object::ObjectId;

  static double constexpr kDistanceLimitInMeters = 150;
  static size_t constexpr kMaxSelectedElements = 3;

  explicit SponsoredDataset(std::string const & dataPath);

  /// @return true if |fb| satisfies some necessary conditions to match one or serveral
  /// objects from dataset.
  bool NecessaryMatchingConditionHolds(feature::FeatureBuilder const & fb) const;
  ObjectId FindMatchingObjectId(feature::FeatureBuilder const & e) const;

  // Applies changes to a given osm object (for example, remove hotel type)
  // and passes the result to |fn|.
  void PreprocessMatchedOsmObject(ObjectId matchedObjId, feature::FeatureBuilder & fb,
                                  std::function<void(feature::FeatureBuilder &)> const fn) const;
  // Creates objects and adds them to the map (MWM) via |fn|.
  void BuildOsmObjects(std::function<void(feature::FeatureBuilder &)> const & fn) const;

  SponsoredObjectStorage<Object> const & GetStorage() const { return m_storage; }

private:
  void BuildObject(Object const & object,
                   std::function<void(feature::FeatureBuilder &)> const & fn) const;

  /// @return an id of a matched object or kInvalidObjectId on failure.
  ObjectId FindMatchingObjectIdImpl(feature::FeatureBuilder const & fb) const;

  SponsoredObjectStorage<Object> m_storage;
};
}  // namespace generator

#include "generator/sponsored_dataset_inl.hpp"  // SponsoredDataset implementation.
