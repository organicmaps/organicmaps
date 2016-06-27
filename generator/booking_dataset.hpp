#pragma once

#include "generator/osm_element.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace generator
{
class BookingDataset
{
public:
  double static constexpr kDistanceLimitInMeters = 150;
  size_t static constexpr kMaxSelectedElements = 3;

  // Calculated with tools/python/booking_hotels_quality.py
  double static constexpr kOptimalThreshold = 0.709283;

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
      Language = 11,
      NameLoc = 12,
      AddressLoc = 13,

      Counter
    };

    uint32_t id = 0;
    double lat = 0.0;
    double lon = 0.0;
    string name;
    string address;
    uint32_t stars = 0;
    uint32_t priceCategory = 0;
    double ratingBooking = 0.0;
    double ratingUser = 0.0;
    string descUrl;
    uint32_t type = 0;
    string langCode;
    string nameLoc;
    string addressLoc;

    static constexpr size_t Index(Fields field) { return static_cast<size_t>(field); }
    static constexpr size_t FieldsCount() { return static_cast<size_t>(Fields::Counter); }
    explicit Hotel(string const & src);
  };

  explicit BookingDataset(string const & dataPath);

  bool BookingFilter(OsmElement const & e) const;
  bool TourismFilter(OsmElement const & e) const;

  Hotel const & GetHotel(size_t index) const;
  vector<size_t> GetNearestHotels(double lat, double lon, size_t limit,
                                  double maxDistance = 0.0) const;
  bool MatchByName(string const & osmName, vector<size_t> const & bookingIndexes) const;

  void BuildFeatures(function<void(OsmElement *)> const & fn) const;

  static double ScoreByLinearNormDistance(double distance);

protected:
  vector<Hotel> m_hotels;

  // create the rtree using default constructor
  using TPoint = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using TBox = boost::geometry::model::box<TPoint>;
  using TValue = pair<TBox, size_t>;

  boost::geometry::index::rtree<TValue, boost::geometry::index::quadratic<16>> m_rtree;

  void LoadHotels(string const & path);
  bool MatchWithBooking(OsmElement const & e) const;
  bool Filter(OsmElement const & e, function<bool(OsmElement const &)> const & fn) const;
};

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h);

}  // namespace generator
