#pragma once

#include "storage/index.hpp"

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <array>
#include <chrono>
#include <tuple>
#include <vector>

namespace eye
{
namespace traits
{
namespace impl
{
template <typename T>
auto is_enum_with_count_checker(int) -> decltype(
  T::Count,
  std::is_enum<T> {});

template <typename T>
std::false_type is_enum_with_count_checker(...);
}  // namespace impl

template <typename T>
using is_enum_with_count = decltype(impl::is_enum_with_count_checker<T>(0));
}  // namespace traits

template <typename T>
using EnableIfIsEnumWithCount = std::enable_if_t<traits::is_enum_with_count<T>::value>;

template <typename T, typename R, typename Enable = void>
class Counters;

template <typename T, typename R>
class Counters<T, R, EnableIfIsEnumWithCount<T>>
{
public:
  void Increment(T const key)
  {
    ++m_counters[static_cast<size_t>(key)];
  }

  R Get(T const key) const
  {
    return m_counters[static_cast<size_t>(key)];
  }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Counters, visitor(m_counters, "counters"))

private:
  std::array<R, static_cast<size_t>(T::Count)> m_counters = {};
};

enum class Version : int8_t
{
  Unknown = -1,
  V0 = 0,
  Latest = V0
};

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;

struct Tips
{
  // The order is important.
  // New types must be added before Type::Count item.
  enum class Type : uint8_t
  {
    BookmarksCatalog,
    BookingHotels,
    DiscoverButton,
    MapsLayers,

    Count
  };

  enum class Event : uint8_t
  {
    ActionClicked,
    GotitClicked,

    Count
  };

  struct Info
  {
    DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_eventCounters, "event_counters"),
                    visitor(m_lastShown, "last_shown"))

    Type m_type;
    Counters<Event, uint32_t> m_eventCounters;
    Time m_lastShown;
  };

  DECLARE_VISITOR(visitor(m_shownTips, "last_shown_tips"),
                  visitor(m_totalShownTipsCount, "total_shown_tips_count"),
                  visitor(m_lastShown, "last_shown"))

  std::vector<Info> m_shownTips;
  uint32_t m_totalShownTipsCount = 0;
  Time m_lastShown;
};

struct InfoV0
{
  static Version GetVersion() { return Version::V0; }
  DECLARE_VISITOR(visitor(m_tips, "tips"))

  Tips m_tips;
};

using Info = InfoV0;

inline std::string DebugPrint(Tips::Type const & type)
{
  switch (type)
  {
  case Tips::Type::BookmarksCatalog: return "BookmarksCatalog";
  case Tips::Type::BookingHotels: return "BookingHotels";
  case Tips::Type::DiscoverButton: return "DiscoverButton";
  case Tips::Type::MapsLayers: return "MapsLayers";
  case Tips::Type::Count: return "Count";
  }
}

inline std::string DebugPrint(Tips::Event const & type)
{
  switch (type)
  {
  case Tips::Event::ActionClicked: return "ActionClicked";
  case Tips::Event::GotitClicked: return "GotitClicked";
  case Tips::Event::Count: return "Count";
  }
}
}  // namespace eye
