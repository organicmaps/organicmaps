#pragma once

#include "base/macros.hpp"

#include <string>
#include <vector>

namespace feature
{
class TypesHolder;
}

namespace facebook
{
struct TypeAndlevel
{
  TypeAndlevel(uint32_t type, uint32_t level) : m_type(type), m_level(level) {}

  uint32_t m_type = 0;
  uint32_t m_level = 0;
};

// Class which match feature types and facebook banner ids.
class Ads
{
public:
  static auto constexpr kBannerIdForOtherTypes = "185237551520383_1384653421578784";

  static Ads const & Instance();

  bool HasBanner(feature::TypesHolder const & types) const;
  std::string GetBannerId(feature::TypesHolder const & types) const;

private:
  Ads();
  void AppendEntry(std::vector<std::vector<std::string>> const & types, std::string const & id);
  void SetExcludeTypes(std::vector<std::vector<std::string>> const & types);

  struct TypesToBannerId
  {
    std::vector<TypeAndlevel> m_types;
    std::string m_bannerId;
  };

  std::vector<TypesToBannerId> m_typesToBanners;
  std::vector<TypeAndlevel> m_excludeTypes;

  DISALLOW_COPY_AND_MOVE(Ads);
};
}  // namespace facebook
