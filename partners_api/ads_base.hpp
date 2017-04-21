#pragma once

#include "indexer/ftypes_mapping.hpp"

#include "storage/index.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <unordered_set>

namespace feature
{
class TypesHolder;
}

namespace ads
{
class ContainerBase
{
public:
  virtual ~ContainerBase() = default;
  virtual bool HasBanner(feature::TypesHolder const & types,
                         storage::TCountryId const & countryId,
                         std::string const & userLanguage) const = 0;
  virtual std::string GetBannerId(feature::TypesHolder const & types,
                                  storage::TCountryId const & countryId,
                                  std::string const & userLanguage) const = 0;
  virtual std::string GetBannerIdForOtherTypes() const = 0;
  virtual bool HasSearchBanner() const = 0;
  virtual std::string GetSearchBannerId() const = 0;
};

// Class which matches feature types and banner ids.
class Container : public ContainerBase
{
public:
  Container();

  // ContainerBase overrides:
  bool HasBanner(feature::TypesHolder const & types,
                 storage::TCountryId const & countryId,
                 std::string const & userLanguage) const override;
  std::string GetBannerId(feature::TypesHolder const & types,
                          storage::TCountryId const & countryId,
                          std::string const & userLanguage) const override;
  std::string GetBannerIdForOtherTypes() const override;
  bool HasSearchBanner() const override;
  std::string GetSearchBannerId() const override;

protected:
  void AppendEntry(std::initializer_list<std::initializer_list<char const *>> && types,
                   std::string const & id);
  void AppendExcludedTypes(std::initializer_list<std::initializer_list<char const *>> && types);
  void AppendSupportedCountries(std::initializer_list<storage::TCountryId> const & countries);
  void AppendSupportedUserLanguages(std::initializer_list<std::string> const & languages);

private:
  ftypes::HashMapMatcher<uint32_t, std::string> m_typesToBanners;
  ftypes::HashSetMatcher<uint32_t> m_excludedTypes;
  // All countries are supported when empty.
  std::unordered_set<storage::TCountryId> m_supportedCountries;
  // It supplements |m_supportedCountries|. If a country isn't supported
  // we check user's language.
  std::unordered_set<int8_t> m_supportedUserLanguages;

  DISALLOW_COPY(Container);
};
}  // namespace ads
