#pragma once

#include "base/assert.hpp"

#include <string>

namespace ads
{
struct Banner
{
  enum class Place : uint8_t
  {
    Search = 0,
    Poi = 1,
    DownloadOnMap = 2,
  };

  enum class Type : uint8_t
  {
    None = 0,
    Facebook = 1,
    RB = 2,
    Mopub = 3,
    TinkoffAllAirlines = 4,
    TinkoffInsurance = 5,
    Mts = 6,
    Skyeng = 7,
    BookmarkCatalog = 8,
  };

  Banner() = default;
  Banner(Type t, std::string const & value) : m_type(t), m_value(value) {}

  Type m_type = Type::None;
  std::string m_value;
};

inline std::string DebugPrint(Banner::Type type)
{
  switch (type)
  {
  case Banner::Type::None: return "None";
  case Banner::Type::Facebook: return "Facebook";
  case Banner::Type::RB: return "RB";
  case Banner::Type::Mopub: return "Mopub";
  case Banner::Type::TinkoffAllAirlines: return "TinkoffAllAirlines";
  case Banner::Type::TinkoffInsurance: return "TinkoffInsurance";
  case Banner::Type::Mts: return "Mts";
  case Banner::Type::Skyeng: return "Skyeng";
  case Banner::Type::BookmarkCatalog: return "BookmarkCatalog";
  }
  UNREACHABLE();
}
}  // namespace ads
