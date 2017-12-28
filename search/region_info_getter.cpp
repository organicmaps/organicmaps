#include "search/region_info_getter.hpp"

#include "storage/country_decl.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstddef>
#include <vector>

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
void GetPathToRoot(storage::TCountryId const & id, storage::TCountryTree const & countries,
                   Fn && fn)
{
  vector<storage::TCountryTree::Node const *> nodes;
  countries.Find(id, nodes);

  if (nodes.empty())
    LOG(LERROR, ("Can't find node in the countries tree for:", id));

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
  storage::TMappingAffiliations affiliations;
  storage::LoadCountriesFromFile(COUNTRIES_FILE, m_countries, affiliations);
}

void RegionInfoGetter::SetLocale(string const & locale)
{
  m_nameGetter = platform::GetTextByIdFactory(platform::TextSource::Countries, locale);
}

string RegionInfoGetter::GetLocalizedFullName(storage::TCountryId const & id) const
{
  size_t const kMaxNumParts = 2;

  vector<string> parts;
  GetPathToRoot(id, m_countries, [&](storage::TCountryId const & id) {
    parts.push_back(GetLocalizedCountryName(id));
  });

  if (parts.size() > kMaxNumParts)
    parts.erase(parts.begin(), parts.end() - kMaxNumParts);

  my::EraseIf(parts, [&](string const & s) { return s.empty(); });

  if (!parts.empty())
    return strings::JoinStrings(parts, ", ");

  // Tries to get at least localized name for |id|, if |id| is a
  // disputed territory.
  auto name = GetLocalizedCountryName(id);
  if (!name.empty())
    return name;

  // Tries to transform map name to the full name.
  name = id;
  storage::CountryInfo::FileName2FullName(name);
  if (!name.empty())
    return name;

  return {};
}

string RegionInfoGetter::GetLocalizedCountryName(storage::TCountryId const & id) const
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
