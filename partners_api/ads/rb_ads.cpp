#include "partners_api/ads/rb_ads.hpp"

namespace
{
auto const kFoodPlacementId = "1";
auto const kShopsPlacementId = "2";
auto const kCityTransportPlacementId = "13";
auto const kGlobalTransportPlacementId = "12";
auto const kSightsPlacementId = "5";
auto const kLargeToponymsPlacementId = "6";
auto const kHealthPlacementId = "7";
auto const kFinancialPlacementId = "8";
auto const kEntertainmentPlacementId = "9";
auto const kBuildingPlacementId = "11";
auto const kBannerIdForOtherTypes = "14";

std::initializer_list<storage::CountryId> const kSupportedCountries = {
    "Azerbaijan Region",  "Armenia",    "Belarus",      "Kazakhstan", "Kyrgyzstan", "Moldova",
    "Russian Federation", "Tajikistan", "Turkmenistan", "Uzbekistan", "Ukraine"};

std::initializer_list<std::string> const kSupportedLanguages = {"be", "hy", "kk", "ru", "uk"};
}  // namespace

namespace ads
{
Rb::Rb()
{
  AppendEntry({{"amenity", "cafe"},
               {"amenity", "fast_food"},
               {"amenity", "restaurant"},
               {"amenity", "bar"},
               {"amenity", "pub"}},
              kFoodPlacementId);

  AppendEntry({{"shop"},
               {"amenity", "marketplace"}},
              kShopsPlacementId);

  AppendEntry({{"aerialway"},
               {"highway", "bus_stop"},
               {"highway", "speed_camera"},
               {"public_transport"}},
              kCityTransportPlacementId);

  AppendEntry({{"aeroway"},
               {"railway"},
               {"man_made", "pier"}},
              kGlobalTransportPlacementId);

  AppendEntry({{"tourism", "zoo"},
               {"tourism", "artwork"},
               {"tourism", "information"},
               {"tourism", "attraction"},
               {"tourism", "viewpoint"},
               {"tourism", "museum"},
               {"amenity", "fountain"},
               {"amenity", "townhall"},
               {"historic"}},
              kSightsPlacementId);

  AppendEntry({{"place"}}, kLargeToponymsPlacementId);

  AppendEntry({{"amenity", "dentist"},
               {"amenity", "doctors"},
               {"amenity", "clinic"},
               {"amenity", "hospital"},
               {"amenity", "pharmacy"},
               {"amenity", "veterinary"}},
              kHealthPlacementId);

  AppendEntry({{"amenity", "bank"},
               {"amenity", "atm"},
               {"amenity", "bureau_de_change"}},
              kFinancialPlacementId);

  AppendEntry({{"amenity", "cinema"},
               {"amenity", "brothel"},
               {"amenity", "casino"},
               {"amenity", "nightclub"},
               {"amenity", "theatre"},
               {"boundary", "national_park"},
               {"leisure"}},
              kEntertainmentPlacementId);

  AppendEntry({{"building"}}, kBuildingPlacementId);

  AppendSupportedCountries(kSupportedCountries);
  AppendSupportedUserLanguages(kSupportedLanguages);
}

std::string Rb::GetBannerForOtherTypes() const
{
  return kBannerIdForOtherTypes;
}
}  // namespace ads
