#pragma once

#include "indexer/index.hpp"

#include "search/reverse_geocoder.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "std/function.hpp"
#include "std/limits.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

class FeatureBuilder1;

namespace generator
{
class BookingDataset
{
public:
  static double constexpr kDistanceLimitInMeters = 150;
  static size_t constexpr kMaxSelectedElements = 3;
  static auto constexpr kInvalidHotelIndex = numeric_limits<uint32_t>::max();

  struct Hotel
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

    // TODO(mgsergio): Make a separate type for this or an alias.
    uint32_t id = 0;
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
    explicit Hotel(string const & src);

    inline bool IsAddressPartsFilled() const { return !street.empty() || !houseNumber.empty(); }
  };

  class AddressMatcher
  {
    Index m_index;
    unique_ptr<search::ReverseGeocoder> m_coder;

  public:
    AddressMatcher();
    void operator()(Hotel & hotel);
  };

  explicit BookingDataset(string const & dataPath, string const & addressReferencePath = string());
  explicit BookingDataset(istream & dataSource, string const & addressReferencePath = string());

  /// @return an index of a matched hotel or kInvalidHotelIndex on failure.
  size_t GetMatchingHotelIndex(FeatureBuilder1 const & fb) const;
  /// @return true if |fb| is a hotel with a name.
  bool CanBeBooking(FeatureBuilder1 const & fb) const;

  inline size_t Size() const { return m_hotels.size(); }
  Hotel const & GetHotelById(uint32_t id) const;
  Hotel & GetHotelById(uint32_t id);
  vector<uint32_t> GetNearestHotels(ms::LatLon const & latLon, size_t limit,
                                    double maxDistance = 0.0) const;
  bool MatchByName(string const & osmName, vector<size_t> const & bookingIndexes) const;

  void BuildHotels(function<void(FeatureBuilder1 &)> const & fn) const;

protected:
  map<uint32_t, Hotel> m_hotels;

  using TPoint = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using TBox = boost::geometry::model::box<TPoint>;
  using TValue = pair<TBox, uint32_t>;

  // Create the rtree using default constructor.
  boost::geometry::index::rtree<TValue, boost::geometry::index::quadratic<16>> m_rtree;

  void BuildHotel(Hotel const & hotel, function<void(FeatureBuilder1 &)> const & fn) const;

  void LoadHotels(istream & path, string const & addressReferencePath);
  /// @return an index of a matched hotel or numeric_limits<size_t>::max() on failure.
  size_t MatchWithBooking(FeatureBuilder1 const & e) const;
};

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h);
}  // namespace generator
