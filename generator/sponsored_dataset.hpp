#pragma once

#include "generator/sponsored_storage.hpp"

#include "base/newtype.hpp"

#include <functional>
#include <iostream>
#include <string>

class FeatureBuilder1;

namespace generator
{
template <typename SponsoredObject>
class SponsoredDataset
{
public:
  using Object = SponsoredObject;
  using ObjectId = typename Object::ObjectId;

  static double constexpr kDistanceLimitInMeters = 150;
  static size_t constexpr kMaxSelectedElements = 3;

  explicit SponsoredDataset(std::string const & dataPath,
                            std::string const & addressReferencePath = std::string());
  explicit SponsoredDataset(std::istream & dataSource,
                            std::string const & addressReferencePath = std::string());

  /// @return true if |fb| satisfies some necessary conditions to match one or serveral
  /// objects from dataset.
  bool NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const;
  ObjectId FindMatchingObjectId(FeatureBuilder1 const & e) const;

  // Applies changes to a given osm object (for example, remove hotel type)
  // and passes the result to |fn|.
  void PreprocessMatchedOsmObject(ObjectId matchedObjId, FeatureBuilder1 & fb,
                                  std::function<void(FeatureBuilder1 &)> const fn) const;
  // Creates objects and adds them to the map (MWM) via |fn|.
  void BuildOsmObjects(std::function<void(FeatureBuilder1 &)> const & fn) const;

private:
  void InitStorage();
  SponsoredStorage<Object> const & GetStorage() const;
  SponsoredStorage<Object> & GetStorage();

  void BuildObject(Object const & object,
                   std::function<void(FeatureBuilder1 &)> const & fn) const;

  /// @return an id of a matched object or kInvalidObjectId on failure.
  ObjectId FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const;

  SponsoredStorage<Object> m_storage;
};
}  // namespace generator

#include "generator/sponsored_dataset_inl.hpp"  // SponsoredDataset implementation.
