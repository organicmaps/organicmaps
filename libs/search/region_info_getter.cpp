#include "search/region_info_getter.hpp"

#include "storage/country_decl.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace search
{
using namespace std;

namespace
{
// Calls |fn| on each node name on the way from |id| to the root of
// the |countries| tree, except the root. Does nothing if there are
// multiple ways from |id| to the |root| (disputed territory).
template <typename Fn>
void GetPathToRoot(storage::CountryId const & id, storage::CountryTree const & countries, Fn && fn)
{
  storage::CountryTree::NodesBufferT nodes;
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
  }
  while (!cur->IsRoot());
}
}  // namespace

void RegionInfoGetter::LoadCountriesTree()
{
  using namespace storage;
  Affiliations affiliations;
  CountryNameSynonyms countryNameSynonyms;
  MwmTopCityGeoIds mwmTopCityGeoIds;
  MwmTopCountryGeoIds mwmTopCountryGeoIds;
  LoadCountriesFromFile(COUNTRIES_FILE, m_countries, affiliations, countryNameSynonyms, mwmTopCityGeoIds,
                        mwmTopCountryGeoIds);
}

void RegionInfoGetter::SetLocale(string const & locale)
{
  m_nameGetter = platform::GetTextByIdFactory(platform::TextSource::Countries, locale);
}

void RegionInfoGetter::GetLocalizedFullName(storage::CountryId const & id, NameBufferT & nameParts) const
{
  buffer_vector<storage::CountryId const *, 4> ids;
  GetPathToRoot(id, m_countries, [&ids](storage::CountryId const & id) { ids.push_back(&id); });

  if (!ids.empty())
  {
    // Avoid City in region info because it will be calculated correctly by cities boundary info.
    // We need only State / Region / Область + Country only.
    // https://github.com/organicmaps/organicmaps/issues/857
    /// @todo Need to invent some common solution, not this Australia hack.
    size_t const numParts = (*ids.back() == "Australia" ? 1 : 2);

    size_t const count = ids.size();
    for (size_t i = (count >= numParts ? count - numParts : 0); i < count; ++i)
    {
      auto name = GetLocalizedCountryName(*ids[i]);
      if (!name.empty())
        nameParts.push_back(std::move(name));
    }

    return;
  }

  // Try to get at least localized name for |id|, if it is a disputed territory.
  auto name = GetLocalizedCountryName(id);
  if (!name.empty())
  {
    nameParts.push_back(std::move(name));
    return;
  }

  // Try to transform map name to the full name.
  name = id;
  storage::CountryInfo::FileName2FullName(name);
  if (!name.empty())
    nameParts.push_back(std::move(name));
}

string RegionInfoGetter::GetLocalizedFullName(storage::CountryId const & id) const
{
  NameBufferT parts;
  GetLocalizedFullName(id, parts);

  return strings::JoinStrings(parts, ", ");
}

string RegionInfoGetter::GetLocalizedCountryName(storage::CountryId const & id) const
{
  if (!m_nameGetter)
    return {};

  auto shortName = (*m_nameGetter)(id + " Short");
  if (!shortName.empty())
    return shortName;

  auto officialName = (*m_nameGetter)(id);
  if (!officialName.empty())
    return officialName;

  return {};
}
}  // namespace search
