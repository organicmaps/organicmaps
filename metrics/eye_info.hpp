#pragma once

#include "storage/index.hpp"

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <array>
#include <cstdint>
#include <chrono>
#include <string>
#include <type_traits>
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

struct Booking
{
  DECLARE_VISITOR(visitor(m_lastFilterUsedTime, "last_filter_used_time"))

  Time m_lastFilterUsedTime;
};

struct Bookmarks
{
  DECLARE_VISITOR(visitor(m_lastOpenedTime, "last_use_time"))

  Time m_lastOpenedTime;
};

struct Discovery
{
  enum class Event
  {
    HotelsClicked,
    AttractionsClicked,
    CafesClicked,
    LocalsClicked,

    MoreHotelsClicked,
    MoreAttractionsClicked,
    MoreCafesClicked,
    MoreLocalsClicked,

    Count
  };

  DECLARE_VISITOR(visitor(m_eventCounters, "event_counters"),
                  visitor(m_lastOpenedTime, "last_opened_time"),
                  visitor(m_lastClickedTime, "last_clicked_time"))

  Counters<Event, uint32_t> m_eventCounters;
  Time m_lastOpenedTime;
  Time m_lastClickedTime;
};

struct Layer
{
  enum class Type : uint8_t
  {
    TrafficJams,
    PublicTransport
  };

  DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_useCount, "use_count"),
                  visitor(m_lastTimeUsed, "last_time_used"))

  Type m_type;
  uint64_t m_useCount = 0;
  Time m_lastTimeUsed;
};

using Layers = std::vector<Layer>;

struct Tip
{
  // The order is important.
  // New types must be added before Type::Count item.
  enum class Type : uint8_t
  {
    BookmarksCatalog,
    BookingHotels,
    DiscoverButton,
    PublicTransport,

    Count
  };

  enum class Event : uint8_t
  {
    ActionClicked,
    GotitClicked,

    Count
  };

  DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_eventCounters, "event_counters"),
                  visitor(m_lastShownTime, "last_shown_time"))

  Type m_type;
  Counters<Event, uint32_t> m_eventCounters;
  Time m_lastShownTime;
};

using Tips = std::vector<Tip>;

struct InfoV0
{
  static Version GetVersion() { return Version::V0; }
  DECLARE_VISITOR(visitor(m_booking, "booking"), visitor(m_bookmarks, "bookmarks"),
                  visitor(m_discovery, "discovery"), visitor(m_layers, "layers"),
                  visitor(m_tips, "tips"))

  Booking m_booking;
  Bookmarks m_bookmarks;
  Discovery m_discovery;
  Layers m_layers;
  Tips m_tips;
};

using Info = InfoV0;

inline std::string DebugPrint(Tip::Type const & type)
{
  switch (type)
  {
  case Tip::Type::BookmarksCatalog: return "BookmarksCatalog";
  case Tip::Type::BookingHotels: return "BookingHotels";
  case Tip::Type::DiscoverButton: return "DiscoverButton";
  case Tip::Type::PublicTransport: return "PublicTransport";
  case Tip::Type::Count: return "Count";
  }
}

inline std::string DebugPrint(Tip::Event const & type)
{
  switch (type)
  {
  case Tip::Event::ActionClicked: return "ActionClicked";
  case Tip::Event::GotitClicked: return "GotitClicked";
  case Tip::Event::Count: return "Count";
  }
}

inline std::string DebugPrint(Layer::Type const & type)
{
  switch (type)
  {
  case Layer::Type::TrafficJams: return "TrafficJams";
  case Layer::Type::PublicTransport: return "PublicTransport";
  }
}
}  // namespace eye
