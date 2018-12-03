#pragma once

#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/visitor.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
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
    CHECK_NOT_EQUAL(key, T::Count, ());
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

inline std::string DebugPrint(Version const & version)
{
  switch (version)
  {
    case Version::Unknown: return "Unknown";
    case Version::V0: return "V0";
  }
}

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;

struct Booking
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(Booking, visitor(m_lastFilterUsedTime, "last_filter_used_time"))

  Time m_lastFilterUsedTime;
};

struct Bookmarks
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(Bookmarks, visitor(m_lastOpenedTime, "last_use_time"))

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

  DECLARE_VISITOR_AND_DEBUG_PRINT(Discovery, visitor(m_eventCounters, "event_counters"),
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

  DECLARE_VISITOR_AND_DEBUG_PRINT(Layer, visitor(m_type, "type"), visitor(m_useCount, "use_count"),
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

  DECLARE_VISITOR_AND_DEBUG_PRINT(Tip, visitor(m_type, "type"),
                                  visitor(m_eventCounters, "event_counters"),
                                  visitor(m_lastShownTime, "last_shown_time"))

  Type m_type;
  Counters<Event, uint32_t> m_eventCounters;
  Time m_lastShownTime;
};

using Tips = std::vector<Tip>;

class MapObject
{
public:
  struct Event
  {
    enum class Type : uint8_t
    {
      Open,
      AddToBookmark,
      UgcEditorOpened,
      UgcSaved,
      RouteToCreated
    };

    DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_userPos, "user_pos"),
                    visitor(m_eventTime, "event_time"));

    Type m_type;
    m2::PointD m_userPos;
    Time m_eventTime;
  };

  using Events = std::deque<Event>;

  MapObject() = default;

  MapObject(std::string const & bestType, m2::PointD const & pos, std::string const & readableName)
    : m_bestType(bestType)
    , m_pos(pos)
    , m_readableName(readableName)
    , m_limitRect(MercatorBounds::ClampX(pos.x - kMwmPointAccuracy),
                  MercatorBounds::ClampY(pos.y - kMwmPointAccuracy),
                  MercatorBounds::ClampX(pos.x + kMwmPointAccuracy),
                  MercatorBounds::ClampY(pos.y + kMwmPointAccuracy))
  {
  }

  bool operator==(MapObject const & rhs) const
  {
    return GetPos() == rhs.GetPos() && GetBestType() == rhs.GetBestType() &&
           GetDefaultName() == rhs.GetDefaultName();
  }

  bool operator!=(MapObject const & rhs) const { return !((*this) == rhs); }

  bool AlmostEquals(MapObject const & rhs) const
  {
    return GetPos().EqualDxDy(rhs.GetPos(), kMwmPointAccuracy) &&
           GetBestType() == rhs.GetBestType() && GetDefaultName() == rhs.GetDefaultName();
  }

  std::string const & GetBestType() const { return m_bestType; }

  void SetBestType(std::string const & bestType) { m_bestType = bestType; }

  m2::PointD const & GetPos() const { return m_pos; }

  void SetPos(m2::PointD const & pos)
  {
    m_pos = pos;
    m_limitRect = MercatorBounds::RectByCenterXYAndOffset(pos, kMwmPointAccuracy);
  }

  std::string const & GetDefaultName() const { return m_defaultName; }

  void SetDefaultName(std::string const & defaultName) { m_defaultName = defaultName; }

  std::string const & GetReadableName() const { return m_readableName; }

  void SetReadableName(std::string const & readableName) { m_readableName = readableName; }

  MapObject::Events & GetEditableEvents() const { return m_events; }

  MapObject::Events const & GetEvents() const { return m_events; }

  m2::RectD GetLimitRect() const { return m_limitRect; }

  bool IsEmpty() const { return m_bestType.empty(); }

  DECLARE_VISITOR(visitor(m_bestType, "type"), visitor(m_pos, "pos"),
                  visitor(m_readableName, "readable_name"), visitor(m_defaultName, "default_name"),
                  visitor(m_events, "events"));

private:
  std::string m_bestType;
  m2::PointD m_pos;
  std::string m_defaultName;
  std::string m_readableName;
  // Mutable because of interface of the m4::Tree provides constant references in ForEach methods,
  // but we need to add events into existing objects to avoid some overhead (copy + change +
  // remove + insert operations). The other solution is to use const_cast in ForEach methods.
  mutable MapObject::Events m_events;
  m2::RectD m_limitRect;
};

using MapObjects = m4::Tree<MapObject>;

struct InfoV0
{
  static Version GetVersion() { return Version::V0; }
  DECLARE_VISITOR_AND_DEBUG_PRINT(InfoV0, visitor(m_booking, "booking"),
                                  visitor(m_bookmarks, "bookmarks"),
                                  visitor(m_discovery, "discovery"), visitor(m_layers, "layers"),
                                  visitor(m_tips, "tips"))

  Booking m_booking;
  Bookmarks m_bookmarks;
  Discovery m_discovery;
  Layers m_layers;
  Tips m_tips;
  MapObjects m_mapObjects;
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

inline std::string DebugPrint(Discovery::Event const & event)
{
  switch (event)
  {
    case Discovery::Event::HotelsClicked: return "HotelsClicked";
    case Discovery::Event::AttractionsClicked: return "AttractionsClicked";
    case Discovery::Event::CafesClicked: return "CafesClicked";
    case Discovery::Event::LocalsClicked: return "LocalsClicked";
    case Discovery::Event::MoreHotelsClicked: return "MoreHotelsClicked";
    case Discovery::Event::MoreAttractionsClicked: return "MoreAttractionsClicked";
    case Discovery::Event::MoreCafesClicked: return "MoreCafesClicked";
    case Discovery::Event::MoreLocalsClicked: return "MoreLocalsClicked";
    case Discovery::Event::Count: return "Count";
  }
}

inline std::string DebugPrint(MapObject::Event::Type const & type)
{
  switch (type)
  {
  case MapObject::Event::Type::Open: return "Open";
  case MapObject::Event::Type::AddToBookmark: return "AddToBookmark";
  case MapObject::Event::Type::UgcEditorOpened: return "UgcEditorOpened";
  case MapObject::Event::Type::UgcSaved: return "UgcSaved";
  case MapObject::Event::Type::RouteToCreated: return "RouteToCreated";
  }
}
}  // namespace eye
