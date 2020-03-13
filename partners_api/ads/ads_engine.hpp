#pragma once

#include "partners_api/ads/ads_base.hpp"
#include "partners_api/ads/banner.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feature
{
class TypesHolder;
}

namespace ads
{
class Engine
{
public:
  Engine();

  std::vector<Banner> GetPoiBanners(feature::TypesHolder const & types,
                                    storage::CountriesVec const & countryIds,
                                    std::string const & userLanguage) const;
  std::vector<Banner> GetSearchBanners() const;

  void DisableAdProvider(Banner::Type const type, Banner::Place const place);

private:
  template <typename T>
  struct ContainerItem
  {
    ContainerItem(Banner::Type type, std::unique_ptr<T> && container)
      : m_type(type), m_container(std::move(container))
    {
    }
    bool m_enabled = true;
    Banner::Type m_type;
    std::unique_ptr<T> m_container;
  };

  template <typename T>
  void SetAdProviderEnabled(std::vector<ContainerItem<T>> & banners, Banner::Type const type,
                            bool const isEnabled)
  {
    for (auto & item : banners)
    {
      if (item.m_type == type)
      {
        item.m_enabled = isEnabled;
        return;
      }
    }
  }

  std::vector<ContainerItem<PoiContainerBase>> m_poiBanners;
  std::vector<ContainerItem<SearchContainerBase>> m_searchBanners;
};
}  // namespace ads
