#pragma once

#include "indexer/index.hpp"

#include "search/reverse_geocoder.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

class FeatureBuilder1;

namespace generator
{
class BookingDataset
{
public:
  double static constexpr kDistanceLimitInMeters = 150;
  size_t static constexpr kMaxSelectedElements = 3;

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

  /// @returns an index of a matched hotel or numeric_limits<size_t>::max on failure.
  size_t GetMatchingHotelIndex(FeatureBuilder1 const & e) const;
  bool TourismFilter(FeatureBuilder1 const & e) const;

  inline size_t Size() const { return m_hotels.size(); }
  Hotel const & GetHotel(size_t index) const;
  Hotel & GetHotel(size_t index);
  vector<size_t> GetNearestHotels(double lat, double lon, size_t limit,
                                  double maxDistance = 0.0) const;
  bool MatchByName(string const & osmName, vector<size_t> const & bookingIndexes) const;

  void BuildFeature(FeatureBuilder1 const & fb, size_t hotelIndex,
                    function<void(FeatureBuilder1 &)> const & fn) const;

protected:
  vector<Hotel> m_hotels;

  // create the rtree using default constructor
  using TPoint = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using TBox = boost::geometry::model::box<TPoint>;
  using TValue = pair<TBox, size_t>;

  boost::geometry::index::rtree<TValue, boost::geometry::index::quadratic<16>> m_rtree;

  void LoadHotels(istream & path, string const & addressReferencePath);
  /// @returns an index of a matched hotel or numeric_limits<size_t>::max() on failure.
  size_t MatchWithBooking(FeatureBuilder1 const & e) const;
  bool CanBeBooking(FeatureBuilder1 const & e) const;
};

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h);
}  // namespace generator
