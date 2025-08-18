#include "routing/mwm_hierarchy_handler.hpp"

#include <unordered_set>

namespace routing
{
namespace
{
// Time penalty in seconds for crossing the country border.
// We add no penalty for crossing borders of the countries that have officially abolished
// passport and other types of border control at their mutual borders.
inline size_t constexpr kCrossCountryPenaltyS = 60 * 60 * 2;

using CountrySetT = std::unordered_set<std::string_view>;

// The Eurasian Economic Union (EAEU) list of countries.
CountrySetT kEAEU = {"Armenia", "Belarus", "Kazakhstan", "Kyrgyzstan", "Russian Federation"};

// The Schengen Area list of countries.
CountrySetT kSchengenArea = {"Austria", "Belgium",       "Czech Republic", "Denmark",    "Estonia",  "Finland",
                             "France",  "Germany",       "Greece",         "Hungary",    "Iceland",  "Italy",
                             "Latvia",  "Liechtenstein", "Lithuania",      "Luxembourg", "Malta",    "Netherlands",
                             "Norway",  "Poland",        "Portugal",       "Slovakia",   "Slovenia", "Spain",
                             "Sweden",  "Switzerland",   "Croatia"};

std::string_view kIreland = "Ireland";
std::string_view kNorthernIrelandMwm = "UK_Northern Ireland";

// In fact, there is no _total_ border control on major roads between Israel and Palestine (UN boundary), except:
// - Gaza, strict access/barrier restrictions should be mapped, no transit traffic.
// - West bank wall (https://www.openstreetmap.org/relation/1410327), access/barrier restrictions should be mapped.
CountrySetT kIsraelAndPalestine = {"Israel Region", "Palestine Region"};
std::string_view kJerusalemMwm = "Jerusalem";

bool IsInSet(CountrySetT const & theSet, std::string const & country)
{
  return theSet.find(country) != theSet.end();
}
}  // namespace

/// @return Top level hierarchy name for MWMs \a mwmName.
/// @note May be empty for the disputed territories.
std::string GetCountryByMwmName(std::string const & mwmName, CountryParentNameGetterFn const & fn)
{
  std::string country = mwmName;
  while (true)
  {
    if (country.empty())
      break;

    auto parent = fn(country);
    if (parent == COUNTRIES_ROOT)
      break;
    else
      country = std::move(parent);
  }
  return country;
}

MwmHierarchyHandler::MwmHierarchyHandler(std::shared_ptr<NumMwmIds> numMwmIds, CountryParentNameGetterFn parentGetterFn)
  : m_numMwmIds(std::move(numMwmIds))
  , m_countryParentNameGetterFn(std::move(parentGetterFn))
{}

std::string MwmHierarchyHandler::GetMwmName(NumMwmId mwmId) const
{
  return m_numMwmIds->GetFile(mwmId).GetName();
}

std::string MwmHierarchyHandler::GetParentCountry(NumMwmId mwmId) const
{
  if (m_numMwmIds && m_numMwmIds->ContainsFileForMwm(mwmId))
    return GetCountryByMwmName(GetMwmName(mwmId), m_countryParentNameGetterFn);
  return {};
}

std::string const & MwmHierarchyHandler::GetParentCountryCached(NumMwmId mwmId)
{
  /// @todo Possible races here? Because can't say for sure that MwmHierarchyHandler is not used concurrently.
  auto [it, inserted] = m_mwmCountriesCache.emplace(mwmId, "");
  if (inserted)
    it->second = GetParentCountry(mwmId);

  return it->second;
}

bool MwmHierarchyHandler::HasCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
{
  if (mwmId1 == mwmId2)
    return false;

  std::string const mwm1 = GetMwmName(mwmId1);
  std::string const mwm2 = GetMwmName(mwmId2);
  std::string const country1 = GetParentCountryCached(mwmId1);
  std::string const country2 = GetParentCountryCached(mwmId2);

  // If one of the mwms belongs to the territorial dispute we add penalty for crossing its borders.
  if (country1.empty() || country2.empty())
  {
    // Except "Jerusalem".
    if (mwm1 == kJerusalemMwm || mwm2 == kJerusalemMwm)
      return false;
    return true;
  }

  if (country1 == country2)
    return false;

  if ((country1 == kIreland && mwm2 == kNorthernIrelandMwm) || (country2 == kIreland && mwm1 == kNorthernIrelandMwm))
    return false;

  if (IsInSet(kIsraelAndPalestine, country1) && IsInSet(kIsraelAndPalestine, country2))
    return false;

  if (IsInSet(kEAEU, country1) && IsInSet(kEAEU, country2))
    return false;

  return !IsInSet(kSchengenArea, country1) || !IsInSet(kSchengenArea, country2);
}

RouteWeight MwmHierarchyHandler::GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
{
  if (HasCrossBorderPenalty(mwmId1, mwmId2))
    return RouteWeight(kCrossCountryPenaltyS);

  return RouteWeight(0.0);
}
}  // namespace routing
