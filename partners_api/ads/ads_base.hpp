#pragma once

#include "partners_api/ads/ads_utils.hpp"
#include "partners_api/taxi_provider.hpp"

#include "storage/storage_defines.hpp"

#include "indexer/ftypes_mapping.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>

namespace feature
{
class TypesHolder;
}

namespace ads
{
class PoiContainerBase
{
public:
  virtual ~PoiContainerBase() = default;
  virtual std::string GetBanner(feature::TypesHolder const & types,
                                storage::CountriesVec const & countryIds,
                                std::string const & userLanguage) const = 0;

private:
  virtual bool HasBanner(feature::TypesHolder const & types,
                         storage::CountriesVec const & countryIds,
                         std::string const & userLanguage) const = 0;
  virtual std::string GetBannerForOtherTypes() const = 0;
};

// Class which matches feature types and banner ids.
class PoiContainer : public PoiContainerBase,
                     protected WithSupportedLanguages,
                     protected WithSupportedCountries
{
public:
  PoiContainer();

  // PoiContainerBase overrides:
  std::string GetBanner(feature::TypesHolder const & types,
                        storage::CountriesVec const & countryIds,
                        std::string const & userLanguage) const override;

  std::string GetBannerForOtherTypesForTesting() const;

protected:
  void AppendEntry(std::initializer_list<std::initializer_list<char const *>> && types,
                   std::string const & id);
  void AppendExcludedTypes(std::initializer_list<std::initializer_list<char const *>> && types);

private:
  bool HasBanner(feature::TypesHolder const & types, storage::CountriesVec const & countryIds,
                 std::string const & userLanguage) const override;
  std::string GetBannerForOtherTypes() const override;

  ftypes::HashMapMatcher<uint32_t, std::string> m_typesToBanners;
  ftypes::HashSetMatcher<uint32_t> m_excludedTypes;

  DISALLOW_COPY(PoiContainer);
};

class SearchContainerBase
{
public:
  SearchContainerBase() = default;
  virtual ~SearchContainerBase() = default;

  virtual std::string GetBanner() const = 0;

private:
  virtual bool HasBanner() const = 0;

  DISALLOW_COPY(SearchContainerBase);
};

class SearchCategoryContainerBase
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual std::vector<taxi::Provider::Type> GetTaxiProvidersAtPos(m2::PointD const & pos) const = 0;
  };

  explicit SearchCategoryContainerBase(Delegate const & delegate);
  virtual ~SearchCategoryContainerBase() = default;

  virtual std::string GetBanner(std::optional<m2::PointD> const & userPos) const = 0;

protected:
  Delegate const & m_delegate;

  DISALLOW_COPY(SearchCategoryContainerBase);
};

class DownloadOnMapContainer : protected WithSupportedLanguages,
                               protected WithSupportedCountries,
                               protected WithSupportedUserPos
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual storage::CountryId GetCountryId(m2::PointD const & pos) = 0;
    virtual storage::CountryId GetTopmostParentFor(storage::CountryId const & countryId) const = 0;
    virtual std::string GetLinkForCountryId(storage::CountryId const & countryId) const = 0;
  };

  explicit DownloadOnMapContainer(Delegate & delegate);
  virtual ~DownloadOnMapContainer() = default;

  virtual std::string GetBanner(storage::CountryId const & countryId,
                                std::optional<m2::PointD> const & userPos,
                                std::string const & userLanguage) const;

protected:
  Delegate & m_delegate;

private:
  bool HasBanner(storage::CountryId const & countryId, std::optional<m2::PointD> const & userPos,
                 std::string const & userLanguage) const;
  virtual std::string GetBannerInternal() const;

  DISALLOW_COPY(DownloadOnMapContainer);
};
}  // namespace ads
