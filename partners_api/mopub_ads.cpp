#include "partners_api/mopub_ads.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kTourismPlacementId = "29c1bc85b46442b5a370552916aa6822";
  auto const kNavigationPlacementId = "00af522ea7f94b77b6c671c7e1b13c3f";
  auto const kNonTourismPlacementId = "67ebcbd0af8345f18cccfb230ca08a17";
  auto const kSponsoredBannerPlacementId = "e0c1d4d9c88c4670b9541116bcb75c5f";
#else
  auto const kTourismPlacementId = "d298f205fb8a47aaafb514d2b5b8cf55";
  auto const kNavigationPlacementId = "fbd54c31a20347a6b5d6654510c542a4";
  auto const kNonTourismPlacementId = "94b8d70370a643929aa4c8c764d25e5b";
  auto const kSponsoredBannerPlacementId = "2bab47102d38485996788ab9b602ce2c";
#endif
}  // namespace

namespace ads
{
Mopub::Mopub()
{
  AppendEntry({{"amenity", "cafe"},       // food
               {"amenity", "fast_food"},
               {"amenity", "restaurant"},
               {"amenity", "bar"},
               {"amenity", "pub"},
               {"shop"},                  // shops
               {"amenity", "marketplace"},
               {"tourism", "hotel"},      // hotels
               {"tourism", "hostel"},
               {"tourism", "motel"},
               {"tourism", "apartment"},
               {"tourism", "resort"},
               {"tourism", "chalet"},
               {"tourism", "zoo"},        // sights
               {"tourism", "artwork"},
               {"tourism", "information"},
               {"tourism", "attraction"},
               {"tourism", "viewpoint"},
               {"tourism", "museum"},
               {"amenity", "fountain"},
               {"amenity", "townhall"},
               {"historic"},
               {"amenity", "cinema"},     // entertainment
               {"amenity", "brothel"},
               {"amenity", "casino"},
               {"amenity", "nightclub"},
               {"amenity", "theatre"},
               {"boundary", "national_park"},
               {"leisure"}},
              kTourismPlacementId);

  AppendEntry({{"building"},              // building
               {"place"},                 // large toponyms
               {"aerialway"},             // city transport
               {"highway", "bus_stop"},
               {"highway", "speed_camera"},
               {"public_transport"},
               {"aeroway"},               // global transport
               {"railway"},
               {"man_made", "pier"}},
              kNavigationPlacementId);

  AppendEntry({{"amenity", "dentist"},    // health
               {"amenity", "doctors"},
               {"amenity", "clinic"},
               {"amenity", "hospital"},
               {"amenity", "pharmacy"},
               {"amenity", "veterinary"},
               {"amenity", "bank"},       // finansial
               {"amenity", "atm"},
               {"amenity", "bureau_de_change"}},
              kNonTourismPlacementId);

  AppendEntry({{"sponsored", "banner"}}, kSponsoredBannerPlacementId);
}

std::string Mopub::GetBannerIdForOtherTypes() const
{
  return kNonTourismPlacementId;
}
}  // namespace ads
