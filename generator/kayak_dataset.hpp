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
  uint32_t m_placeID = 0;
};

class KayakPlace
{
  enum Fields
  {
    CountryCode = 0,
    CountryFileName,
    CountryName,
    HasHotels,
    HasImage,
    Hierarchy,
    IataCode,
    KayakCityID,
    KayakPlaceID,
    Latitude,
    Longitude,
    NumberOfHotels,
    PlaceFileName,
    PlaceID,
    PlaceName,
    PlaceType,
    Searchable,

    Counter
  };

public:
  explicit KayakPlace(std::string src);

  friend std::string DebugPrint(KayakPlace const & p);

  uint32_t m_placeID, m_kayakPlaceID;
  bool m_good = false;
};

using BaseDatasetT = SponsoredDataset<KayakHotel>;
class KayakDataset : public BaseDatasetT
{
  std::unordered_map<uint32_t, uint32_t> m_place2kayak;

public:
  KayakDataset(std::string const & hotelsPath, std::string const & placesPath);

  uint32_t GetKayakPlaceID(uint32_t placeID) const
  {
    auto it = m_place2kayak.find(placeID);
    return it != m_place2kayak.end() ? it->second : 0;
  }
};

}  // namespace generator
