#pragma once

#include "indexer/index.hpp"

#include "search/reverse_geocoder.hpp"

#include "base/newtype.hpp"

#include "std/function.hpp"
#include "std/limits.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

class FeatureBuilder1;

namespace generator
{
class SponsoredDataset
{
public:
  NEWTYPE(uint32_t, ObjectId);

  static double constexpr kDistanceLimitInMeters = 150;
  static size_t constexpr kMaxSelectedElements = 3;
  static ObjectId const kInvalidObjectId;

  struct Object
  {
    enum class Fields
    {
      Id = 0,
      Latitude = 1,
      Longtitude = 2,
      Name = 3,
      Address = 4,
      Stars = 5,
      PriceCategory = 6,
      RatingBooking = 7,
      RatingUsers = 8,
      DescUrl = 9,
      Type = 10,
      Translations = 11,

      Counter
    };

    ObjectId id{kInvalidObjectId};
    double lat = 0.0;
    double lon = 0.0;
    string name;
    string address;
    string street;
    string houseNumber;
    uint32_t stars = 0;
    uint32_t priceCategory = 0;
    double ratingBooking = 0.0;
    double ratingUser = 0.0;
    string descUrl;
    uint32_t type = 0;
    string translations;

    static constexpr size_t Index(Fields field) { return static_cast<size_t>(field); }
    static constexpr size_t FieldsCount() { return static_cast<size_t>(Fields::Counter); }
    explicit Object(string const & src);

    inline bool IsAddressPartsFilled() const { return !street.empty() || !houseNumber.empty(); }
  };

  class AddressMatcher
  {
    Index m_index;
    unique_ptr<search::ReverseGeocoder> m_coder;

  public:
    AddressMatcher();
    void operator()(Object & object);
  };

  virtual ~SponsoredDataset() = default;

  /// @return an id of a matched object or kInvalidObjectId on failure.
  virtual ObjectId FindMatchingObjectId(FeatureBuilder1 const & fb) const = 0;

  virtual size_t Size() const = 0;

  virtual void BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const = 0;
};

ostream & operator<<(ostream & s, SponsoredDataset::Object const & h);

NEWTYPE_SIMPLE_OUTPUT(SponsoredDataset::ObjectId);

class SponsoredDatasetBase : public SponsoredDataset
{
public:
  explicit SponsoredDatasetBase(string const & dataPath, string const & addressReferencePath = string());
  explicit SponsoredDatasetBase(istream & dataSource, string const & addressReferencePath = string());

  size_t Size() const override { return m_hotels.size(); }

  Object const & GetObjectById(ObjectId id) const;
  Object & GetObjectById(ObjectId id);
  vector<ObjectId> GetNearestObjects(ms::LatLon const & latLon, size_t limit,
                                     double maxDistance = 0.0) const;

  /// @return true if |fb| satisfies some necesary conditions to match one or serveral
  /// objects from dataset.
  virtual bool NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const = 0;
  ObjectId FindMatchingObjectId(FeatureBuilder1 const & e) const override;

  void BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const override;

protected:
  map<ObjectId, Object> m_hotels;

  using TPoint = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using TBox = boost::geometry::model::box<TPoint>;
  using TValue = pair<TBox, ObjectId>;

  // Create the rtree using default constructor.
  boost::geometry::index::rtree<TValue, boost::geometry::index::quadratic<16>> m_rtree;

  virtual void BuildObject(Object const & object, function<void(FeatureBuilder1 &)> const & fn) const = 0;

  void LoadData(istream & src, string const & addressReferencePath);

  /// @return an id of a matched object or kInvalidObjectId on failure.
  virtual ObjectId FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const = 0;
};
}  // namespace generator
