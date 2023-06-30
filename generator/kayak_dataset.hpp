#pragma once

#include "generator/sponsored_dataset.hpp"
#include "generator/sponsored_object_base.hpp"

#include <string>

namespace generator
{
class KayakHotel : public SponsoredObjectBase
{
  enum Fields
  {
    ChainID = 0,
    ChainName,
    Checkin,
    Checkout,
    CountryCode,
    CountryFileName,
    CountryName,
    CurrencyCode,
    DateCreated,
    Facilities,
    HotelAddress,
    HotelFileName,
    HotelID,
    HotelName,
    HotelPostcode,
    IataPlaceCode,
    ImageID,
    KayakHotelID,
    LastUpdated,
    Latitude,
    Longitude,
    MinRate,
    OverallRating,
    PlaceFileName,
    PlaceID,
    PlaceName,
    PlaceType,
    Popularity,
    PropertyType,
    PropertyTypeID,
    SelfRated,
    StarRating,
    StateName,
    StatePlaceID,
    StatePlacefilename,
    Themes,
    Trademarked,
    TransliteratedHotelName,

    Counter
  };

public:
  explicit KayakHotel(std::string src);

  static constexpr size_t FieldsCount() { return Fields::Counter; }

  static double constexpr kInvalidRating = 0;
  double m_overallRating = kInvalidRating;
};

using KayakDataset = SponsoredDataset<KayakHotel>;
}  // namespace generator
