#include "routing/mwm_hierarchy_handler.hpp"

#include "base/logging.hpp"

#include <unordered_set>

namespace routing
{
// Time penalty in seconds for crossing the country border.
// We add no penalty for crossing borders of the countries that have officially abolished
// passport and other types of border control at their mutual borders.
inline size_t constexpr kCrossCountryPenaltyS = 60 * 60 * 2;

// The Eurasian Economic Union (EAEU) list of countries.
std::unordered_set<std::string> kEAEU{"Armenia", "Belarus", "Kazakhstan", "Kyrgyzstan",
                                      "Russian Federation"};

// The Schengen Area list of countries.
std::unordered_set<std::string> kSchengenArea{
    "Austria", "Belgium",       "Czech Republic", "Denmark",    "Estonia",  "Finland",
    "France",  "Germany",       "Greece",         "Hungary",    "Iceland",  "Italy",
    "Latvia",  "Liechtenstein", "Lithuania",      "Luxembourg", "Malta",    "Netherlands",
    "Norway",  "Poland",        "Portugal",       "Slovakia",   "Slovenia", "Spain",
    "Sweden",  "Switzerland"};

// Returns country name for |mwmName|. The logic of searching for a parent is determined by |fn|.
// Country name may be empty.
std::string GetCountryByMwmName(std::string const & mwmName, CountryParentNameGetterFn fn)
{
  static std::string const CountriesRoot = "Countries";
  std::string country;

  if (!fn)
    return country;

  std::string parent = mwmName;

  while (parent != CountriesRoot)
  {
    country = parent;
    if (country.empty())
      break;

    parent = fn(parent);
  }

  return country;
}

std::string GetCountryByMwmId(NumMwmId mwmId, CountryParentNameGetterFn fn,
                              std::shared_ptr<NumMwmIds> const & numMwmIds)
{
  if (numMwmIds != nullptr && numMwmIds->ContainsFileForMwm(mwmId))
    return GetCountryByMwmName(numMwmIds->GetFile(mwmId).GetName(), fn);
  return {};
}

MwmHierarchyHandler::MwmHierarchyHandler(std::shared_ptr<NumMwmIds> numMwmIds,
                                         CountryParentNameGetterFn countryParentNameGetterFn)
  : m_numMwmIds(numMwmIds), m_countryParentNameGetterFn(countryParentNameGetterFn)
{
}

std::string const & MwmHierarchyHandler::GetParentCountryByMwmId(NumMwmId mwmId)
{
  auto [it, inserted] = m_mwmCountriesCache.emplace(mwmId, "");
  if (inserted)
    it->second = GetCountryByMwmId(mwmId, m_countryParentNameGetterFn, m_numMwmIds);

  return it->second;
}

bool MwmHierarchyHandler::HasCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
{
  if (mwmId1 == mwmId2)
    return false;

  std::string const country1 = GetParentCountryByMwmId(mwmId1);
  std::string const country2 = GetParentCountryByMwmId(mwmId2);

  // If one of the mwms belongs to the territorial dispute we add penalty for crossing its borders.
  if (country1.empty() || country2.empty())
    return true;

  if (country1 == country2)
    return false;

  if (kEAEU.find(country1) != kEAEU.end() && kEAEU.find(country2) != kEAEU.end())
    return false;

  return kSchengenArea.find(country1) == kSchengenArea.end() ||
         kSchengenArea.find(country2) == kSchengenArea.end();
}

RouteWeight MwmHierarchyHandler::GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
{
  if (HasCrossBorderPenalty(mwmId1, mwmId2))
    return RouteWeight(kCrossCountryPenaltyS);

  return RouteWeight(0.0);
}
}  // namespace routing
