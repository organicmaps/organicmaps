#include "search/region_info_getter.hpp"

#include "storage/country_decl.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstddef>

using namespace std;
using namespace storage;

namespace search
{
namespace
{
// Calls |fn| on each node name on the way from |id| to the root of
// the |countries| tree, except the root. Does nothing if there are
// multiple ways from |id| to the |root|.
template <typename Fn>
void GetPathToRoot(storage::CountryId const & id, storage::CountryTree const & countries, Fn && fn)
{
  vector<storage::CountryTree::Node const *> nodes;
  countries.Find(id, nodes);

  if (nodes.empty())
    LOG(LWARNING, ("Can't find node in the countries tree for:", id));

  if (nodes.size() != 1 || nodes[0]->IsRoot())
    return;

  auto const * cur = nodes[0];
  do
  {
    fn(cur->Value().Name());
    cur = &cur->Parent();
  } while (!cur->IsRoot());
}
}  // namespace

void RegionInfoGetter::LoadCountriesTree()
{
  storage::Affiliations affiliations;
  storage::CountryNameSynonyms countryNameSynonyms;
  storage::MwmTopCityGeoIds mwmTopCityGeoIds;
  storage::MwmTopCountryGeoIds mwmTopCountryGeoIds;
  storage::LoadCountriesFromFile(COUNTRIES_FILE, m_countries, affiliations, countryNameSynonyms,
                                 mwmTopCityGeoIds, mwmTopCountryGeoIds);
}

void RegionInfoGetter::SetLocale(string const & locale)
{
  m_nameGetter = platform::GetTextByIdFactory(platform::TextSource::Countries, locale);
}

void RegionInfoGetter::GetLocalizedFullName(storage::CountryId const & id,
                                            vector<string> & nameParts) const
{
  size_t const kMaxNumParts = 2;

  GetPathToRoot(id, m_countries, [&](storage::CountryId const & id) {
    nameParts.push_back(GetLocalizedCountryName(id));
  });

  if (nameParts.size() > kMaxNumParts)
    nameParts.erase(nameParts.begin(), nameParts.end() - kMaxNumParts);

  base::EraseIf(nameParts, [&](string const & s) { return s.empty(); });

  if (!nameParts.empty())
    return;

  // Tries to get at least localized name for |id|, if |id| is a
  // disputed territory.
  auto name = GetLocalizedCountryName(id);
  if (!name.empty())
  {
    nameParts.push_back(name);
    return;
  }

  // Tries to transform map name to the full name.
  name = id;
  storage::CountryInfo::FileName2FullName(name);
  if (!name.empty())
    nameParts.push_back(name);
}

string RegionInfoGetter::GetLocalizedFullName(storage::CountryId const & id) const
{
  vector<string> parts;
  GetLocalizedFullName(id, parts);

  return strings::JoinStrings(parts, ", ");
}

string RegionInfoGetter::GetLocalizedCountryName(storage::CountryId const & id) const
{
  if (!m_nameGetter)
    return {};

  auto const shortName = (*m_nameGetter)(id + " Short");
  if (!shortName.empty())
    return shortName;

  auto const officialName = (*m_nameGetter)(id);
  if (!officialName.empty())
    return officialName;

  return {};
}
}  // namespace search
