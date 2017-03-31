#include "partners_api/ads_engine.hpp"
#include "partners_api/facebook_ads.hpp"
#include "partners_api/rb_ads.hpp"

#include "base/stl_add.hpp"

#include <algorithm>

namespace ads
{
Engine::Engine()
{
  // The banner systems are placed by priority. First has a top priority.
  m_containers.emplace_back(Banner::Type::RB, my::make_unique<Rb>());
  m_containers.emplace_back(Banner::Type::Facebook, my::make_unique<Facebook>());
}

bool Engine::HasBanner(feature::TypesHolder const & types) const
{
  return std::any_of(m_containers.cbegin(), m_containers.cend(), [&types](ContainerItem const & item)
  {
    return item.m_container->HasBanner(types);
  });
}

std::vector<Banner> Engine::GetBanners(feature::TypesHolder const & types) const
{
  std::vector<Banner> banners;
  for (auto const & item : m_containers)
  {
    auto const bannerId = item.m_container->GetBannerId(types);
    if (!bannerId.empty())
      banners.emplace_back(item.m_type, bannerId);
  }

  return banners;
}
}  // namespace ads
