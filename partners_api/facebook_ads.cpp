#include "partners_api/facebook_ads.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

namespace
{
using namespace facebook;

template <typename It>
It FindType(feature::TypesHolder const & types, It first, It last)
{
  for (auto const t : types)
  {
    auto const it = std::find_if(first, last, [t](TypeAndlevel const & tl) {
      auto truncatedType = t;
      ftype::TruncValue(truncatedType, tl.m_level);
      return truncatedType == tl.m_type;
    });
    if (it != last)
      return it;
  }
  return last;
}
}  // namespace

namespace facebook
{
Ads::Ads()
{
  // Food.
  AppendEntry({{"amenity", "cafe"},
               {"amenity", "fast_food"},
               {"amenity", "restaurant"},
               {"amenity", "bar"},
               {"amenity", "pub"}},
              "185237551520383_1384650164912443");
  // Shops.
  AppendEntry({{"shop"}}, "185237551520383_1384650804912379");
  // City Transport.
  AppendEntry({{"aerialway"},
               {"highway", "bus_stop"},
               {"highway", "speed_camera"},
               {"public_transport"}},
              "185237551520383_1384651074912352");
  // Global transport.
  AppendEntry({{"aeroway"},
               {"railway"},
               {"man_made", "pier"}},
              "185237551520383_1387632484614211");
  // Hotels.
  AppendEntry({{"tourism", "hotel"},
               {"tourism", "hostel"},
               {"tourism", "motel"},
               {"tourism", "apartment"},
               {"tourism", "resort"}},
              "185237551520383_1384651324912327");
  // Sights.
  AppendEntry({{"tourism", "chalet"},
               {"tourism", "zoo"},
               {"tourism", "artwork"},
               {"tourism", "information"},
               {"tourism", "attraction"},
               {"tourism", "viewpoint"},
               {"tourism", "museum"},
               {"amenity", "fountain"},
               {"amenity", "theatre"},
               {"amenity", "townhall"},
               {"historic"}},
              "185237551520383_1384651734912286");
  // Large toponyms.
  AppendEntry({{"place"}}, "185237551520383_1384652164912243");
  // Health.
  AppendEntry({{"amenity", "dentist"},
               {"amenity", "doctors"},
               {"amenity", "clinic"},
               {"amenity", "hospital"},
               {"amenity", "pharmacy"},
               {"amenity", "veterinary"}},
              "185237551520383_1384652351578891");
  // Financial.
  AppendEntry({{"amenity", "bank"}, {"amenity", "atm"}}, "185237551520383_1384652658245527");

  SetExcludeTypes({{"sponsored", "booking"}});
}

void Ads::AppendEntry(std::vector<std::vector<std::string>> const & types, std::string const & id)
{
  TypesToBannerId entry;
  for (auto const & type : types)
    entry.m_types.emplace_back(classif().GetTypeByPath(type), type.size());
  entry.m_bannerId = id;
  m_typesToBanners.push_back(move(entry));
}

void Ads::SetExcludeTypes(std::vector<std::vector<std::string>> const & types)
{
  for (auto const & type : types)
    m_excludeTypes.emplace_back(classif().GetTypeByPath(type), type.size());
}

// static
Ads const & Ads::Instance()
{
  static Ads const ads;
  return ads;
}

bool Ads::HasBanner(feature::TypesHolder const & types) const
{
  return FindType(types, m_excludeTypes.begin(), m_excludeTypes.end()) == m_excludeTypes.end();
}

std::string Ads::GetBannerId(feature::TypesHolder const & types) const
{
  if (!HasBanner(types))
    return {};

  for (auto const & typesToBanner : m_typesToBanners)
  {
    auto const it = FindType(types, typesToBanner.m_types.begin(), typesToBanner.m_types.end());
    if (it != typesToBanner.m_types.end())
      return typesToBanner.m_bannerId;
  }
  return kBannerIdForOtherTypes;
}
}  // namespace facebook
