#include "partners_api/ads/mopub_ads.hpp"
#include "partners_api/partners.hpp"

#include "std/target_os.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kTourismPlacementId = "563dafba19914059a7d27aa796b1e436";
  auto const kNavigationPlacementId = "f77789b8394d44c49cf28e6441c7fe84";
  auto const kNonTourismPlacementId = "be17abd98ed449ea8e62f2da94a14a62";
  auto const kSponsoredBannerPlacementId = "6e0b9d157b73471cb1dc05b23cf52ac5";
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
               {"amenity", "bank"},       // financial
               {"amenity", "atm"},
               {"amenity", "bureau_de_change"}},
              kNonTourismPlacementId);

  for (auto const & p : GetPartners())
  {
    auto const & placementId = p.GetBannerPlacementId();
    if (!placementId.empty())
      AppendEntry({{"sponsored", p.m_type.c_str()}}, placementId);
  }
}

// static
std::string Mopub::InitializationBannerId()
{
  return kSponsoredBannerPlacementId;
}

std::string Mopub::GetBannerForOtherTypes() const
{
  return kNonTourismPlacementId;
}
}  // namespace ads
