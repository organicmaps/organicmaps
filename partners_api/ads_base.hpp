#pragma once

#include "storage/index.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
                         storage::TCountryId const & countryId) const = 0;
  virtual std::string GetBannerId(feature::TypesHolder const & types,
                                  storage::TCountryId const & countryId) const = 0;
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
                 storage::TCountryId const & countryId) const override;
  std::string GetBannerId(feature::TypesHolder const & types,
                          storage::TCountryId const & countryId) const override;
  std::string GetBannerIdForOtherTypes() const override;
  bool HasSearchBanner() const override;
  std::string GetSearchBannerId() const override;

protected:
  void AppendEntry(std::vector<std::vector<std::string>> const & types, std::string const & id);
  void AppendExcludedTypes(std::vector<std::vector<std::string>> const & types);
  void AppendSupportedCountries(std::vector<storage::TCountryId> const & countries);

private:
  std::unordered_map<uint32_t, std::string> m_typesToBanners;
  std::unordered_set<uint32_t> m_excludedTypes;
  // All countries are supported when empty.
  std::unordered_set<storage::TCountryId> m_supportedCountries;

  DISALLOW_COPY(Container);
};
}  // namespace ads
