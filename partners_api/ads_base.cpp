#include "partners_api/ads_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include <algorithm>

namespace
{
template <typename Container>
typename Container::const_iterator
FindType(feature::TypesHolder const & types, Container const & cont)
{
  for (auto const t : types)
  {
    for (auto level = ftype::GetLevel(t); level; --level)
    {
      auto truncatedType = t;
      ftype::TruncValue(truncatedType, level);
      auto const it = cont.find(truncatedType);

      if (it != cont.end())
        return it;
    }
  }

  return cont.cend();
}
}  // namespace

namespace ads
{
Container::Container() { AppendExcludedTypes({{"sponsored", "booking"}}); }

void Container::AppendEntry(std::vector<std::vector<std::string>> const & types,
                            std::string const & id)
{
  for (auto const & type : types)
  {
#if defined(DEBUG)
    feature::TypesHolder holder;
    holder.Assign(classif().GetTypeByPath(type));
    ASSERT(FindType(holder, m_typesToBanners) == m_typesToBanners.cend(),
           ("Banner id for this type already exists", type));
#endif
    m_typesToBanners.emplace(classif().GetTypeByPath(type), id);
  }
}

void Container::AppendExcludedTypes(std::vector<std::vector<std::string>> const & types)
{
  for (auto const & type : types)
  {
#if defined(DEBUG)
    feature::TypesHolder holder;
    holder.Assign(classif().GetTypeByPath(type));
    ASSERT(FindType(holder, m_excludedTypes) == m_excludedTypes.cend(),
           ("Excluded banner type already exists"));
#endif
    m_excludedTypes.emplace(classif().GetTypeByPath(type));
  }
}

void Container::AppendSupportedCountries(std::vector<storage::TCountryId> const & countries)
{
  m_supportedCountries.insert(countries.begin(), countries.end());
}

bool Container::HasBanner(feature::TypesHolder const & types,
                          storage::TCountryId const & countryId) const
{
  if (!m_supportedCountries.empty() &&
      m_supportedCountries.find(countryId) == m_supportedCountries.end())
  {
    return false;
  }

  return FindType(types, m_excludedTypes) == m_excludedTypes.cend();
}

std::string Container::GetBannerId(feature::TypesHolder const & types,
                                   storage::TCountryId const & countryId) const
{
  if (!HasBanner(types, countryId))
    return {};

  auto const it = FindType(types, m_typesToBanners);
  if (it != m_typesToBanners.cend())
    return it->second;

  return GetBannerIdForOtherTypes();
}

std::string Container::GetBannerIdForOtherTypes() const
{
  return {};
}

bool Container::HasSearchBanner() const
{
  return false;
}

std::string Container::GetSearchBannerId() const
{
  return {};
}
}  // namespace ads
