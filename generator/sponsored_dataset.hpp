#pragma once

#include "indexer/index.hpp"

#include "search/reverse_geocoder.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/newtype.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

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

  explicit SponsoredDataset(string const & dataPath, string const & addressReferencePath = string());
  explicit SponsoredDataset(istream & dataSource, string const & addressReferencePath = string());

  size_t Size() const { return m_objects.size(); }

  Object const & GetObjectById(ObjectId id) const;
  Object & GetObjectById(ObjectId id);
  vector<ObjectId> GetNearestObjects(ms::LatLon const & latLon, size_t limit,
                                     double maxDistance = 0.0) const;

  /// @return true if |fb| satisfies some necessary conditions to match one or serveral
  /// objects from dataset.
  bool NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const;
  ObjectId FindMatchingObjectId(FeatureBuilder1 const & e) const;

  // Applies changes to a given osm object (for example, remove hotel type)
  // and passes the result to |fn|.
  void PreprocessMatchedOsmObject(ObjectId matchedObjId, FeatureBuilder1 & fb,
                                  function<void(FeatureBuilder1 &)> const fn) const;
  // Creates objects and adds them to the map (MWM) via |fn|.
  void BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const;

protected:
  class AddressMatcher
  {
  public:
    AddressMatcher();
    void operator()(Object & object);

  private:
    Index m_index;
    unique_ptr<search::ReverseGeocoder> m_coder;
  };

  // TODO(mgsergio): Get rid of Box since boost::rtree supports point as value type.
  // TODO(mgsergio): Use mercator instead of latlon or boost::geometry::cs::spherical_equatorial
  // instead of boost::geometry::cs::cartesian.
  using Point = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using Box = boost::geometry::model::box<Point>;
  using Value = pair<Box, ObjectId>;

  // Create the rtree using default constructor.
  boost::geometry::index::rtree<Value, boost::geometry::index::quadratic<16>> m_rtree;

  void BuildObject(Object const & object,
                   function<void(FeatureBuilder1 &)> const & fn) const;

  void LoadData(istream & src, string const & addressReferencePath);

  /// @return an id of a matched object or kInvalidObjectId on failure.
  ObjectId FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const;

  map<ObjectId, Object> m_objects;
};
}  // namespace generator

#include "generator/sponsored_dataset_inl.hpp"  // SponsoredDataset implementation.
