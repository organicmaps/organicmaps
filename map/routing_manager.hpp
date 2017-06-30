#pragma once

#include "map/bookmark_manager.hpp"
#include "map/routing_mark.hpp"

#include "routing/route.hpp"
#include "routing/routing_session.hpp"

#include "storage/index.hpp"

#include "drape/pointers.hpp"

#include "tracking/reporter.hpp"

#include "base/thread_checker.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace df
{
class DrapeEngine;
}

namespace storage
{
class CountryInfoGetter;
}

namespace routing
{
class NumMwmIds;
}

class Index;

struct RoutePointInfo
{
  std::string m_name;
  RouteMarkType m_markType = RouteMarkType::Start;
  int8_t m_intermediateIndex = 0;
  bool m_isPassed = false;
  bool m_isMyPosition = false;
  m2::PointD m_position;
};

class RoutingManager final
{
public:
  class Delegate
  {
  public:
    virtual void OnRouteFollow(routing::RouterType type) = 0;
    virtual void RegisterCountryFilesOnRoute(std::shared_ptr<routing::NumMwmIds> ptr) const = 0;

    virtual ~Delegate() = default;
  };

  struct Callbacks
  {
    using FeatureIndexGetterFn = std::function<Index &()>;
    using CountryInfoGetterFn = std::function<storage::CountryInfoGetter &()>;

    Callbacks(FeatureIndexGetterFn && featureIndexGetter, CountryInfoGetterFn && countryInfoGetter)
      : m_featureIndexGetter(std::move(featureIndexGetter))
      , m_countryInfoGetter(std::move(countryInfoGetter))
    {}

    FeatureIndexGetterFn m_featureIndexGetter;
    CountryInfoGetterFn m_countryInfoGetter;
  };

  using RouteBuildingCallback =
      std::function<void(routing::IRouter::ResultCode, storage::TCountriesVec const &)>;
  using RouteProgressCallback = std::function<void(float)>;

  RoutingManager(Callbacks && callbacks, Delegate & delegate);

  void SetBookmarkManager(BookmarkManager * bmManager);

  routing::RoutingSession const & RoutingSession() const { return m_routingSession; }
  routing::RoutingSession & RoutingSession() { return m_routingSession; }
  void SetRouter(routing::RouterType type);
  routing::RouterType GetRouter() const { return m_currentRouterType; }
  bool IsRoutingActive() const { return m_routingSession.IsActive(); }
  bool IsRouteBuilt() const { return m_routingSession.IsBuilt(); }
  bool IsRouteBuilding() const { return m_routingSession.IsBuilding(); }
  bool IsRouteRebuildingOnly() const { return m_routingSession.IsRebuildingOnly(); }
  bool IsRouteNotReady() const { return m_routingSession.IsNotReady(); }
  bool IsRouteFinished() const { return m_routingSession.IsFinished(); }
  bool IsOnRoute() const { return m_routingSession.IsOnRoute(); }
  bool IsRoutingFollowing() const { return m_routingSession.IsFollowing(); }
  void BuildRoute(uint32_t timeoutSec);
  void SetUserCurrentPosition(m2::PointD const & position);
  void ResetRoutingSession() { m_routingSession.Reset(); }
  // FollowRoute has a bug where the router follows the route even if the method hads't been called.
  // This method was added because we do not want to break the behaviour that is familiar to our
  // users.
  bool DisableFollowMode();
  /// @TODO(AlexZ): Warning! These two routing callbacks are the only callbacks which are not called
  /// in the main thread context.
  /// UI code should take it into an account. This is a result of current implementation, that can
  /// be improved:
  /// Drape core calls some RunOnGuiThread with "this" pointers, and it causes crashes on Android,
  /// when Drape engine is destroyed
  /// while switching between activities. Current workaround cleans all callbacks when destroying
  /// Drape engine
  /// (@see MwmApplication.clearFunctorsOnUiThread on Android). Better soulution can be fair copying
  /// of all needed information into
  /// lambdas/functors before calling RunOnGuiThread.
  void SetRouteBuildingListener(RouteBuildingCallback const & buildingCallback)
  {
    m_routingCallback = buildingCallback;
  }
  /// See warning above.
  void SetRouteProgressListener(RouteProgressCallback const & progressCallback)
  {
    m_routingSession.SetProgressCallback(progressCallback);
  }
  void FollowRoute();
  void CloseRouting(bool removeRoutePoints);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const
  {
    m_routingSession.GetRouteFollowingInfo(info);
  }
  m2::PointD GetRouteEndPoint() const { return m_routingSession.GetEndPoint(); }
  /// Returns the most situable router engine type.
  routing::RouterType GetBestRouter(m2::PointD const & startPoint,
                                    m2::PointD const & finalPoint) const;
  routing::RouterType GetLastUsedRouter() const;
  void SetLastUsedRouter(routing::RouterType type);
  // Sound notifications for turn instructions.
  void EnableTurnNotifications(bool enable) { m_routingSession.EnableTurnNotifications(enable); }
  bool AreTurnNotificationsEnabled() const
  {
    return m_routingSession.AreTurnNotificationsEnabled();
  }
  /// \brief Sets a locale for TTS.
  /// \param locale is a string with locale code. For example "en", "ru", "zh-Hant" and so on.
  /// \note See sound/tts/languages.txt for the full list of available locales.
  void SetTurnNotificationsLocale(std::string const & locale)
  {
    m_routingSession.SetTurnNotificationsLocale(locale);
  }
  /// @return current TTS locale. For example "en", "ru", "zh-Hant" and so on.
  /// In case of error returns an empty string.
  /// \note The method returns correct locale after SetTurnNotificationsLocale has been called.
  /// If not, it returns an empty string.
  std::string GetTurnNotificationsLocale() const
  {
    return m_routingSession.GetTurnNotificationsLocale();
  }
  /// \brief When an end user is going to a turn he gets sound turn instructions.
  /// If C++ part wants the client to pronounce an instruction GenerateTurnNotifications (in
  /// turnNotifications) returns
  /// an array of one of more strings. C++ part assumes that all these strings shall be pronounced
  /// by the client's TTS.
  /// For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  /// an array with one string "Make a right turn.". The next call of the method returns nothing.
  /// GenerateTurnNotifications shall be called by the client when a new position is available.
  void GenerateTurnNotifications(std::vector<std::string> & turnNotifications);

  void AddRoutePoint(RouteMarkData && markData);
  std::vector<RouteMarkData> GetRoutePoints() const;
  size_t GetRoutePointsCount() const;
  void RemoveRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  void RemoveIntermediateRoutePoints();
  void MoveRoutePoint(RouteMarkType currentType, int8_t currentIntermediateIndex,
                      RouteMarkType targetType, int8_t targetIntermediateIndex);
  void HideRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  bool CouldAddIntermediatePoint() const;
  bool IsMyPosition(RouteMarkType type, int8_t intermediateIndex = 0);

  void SetRouterImpl(routing::RouterType type);
  void RemoveRoute(bool deactivateFollowing);

  void CheckLocationForRouting(location::GpsInfo const & info);
  void CallRouteBuilded(routing::IRouter::ResultCode code,
                        storage::TCountriesVec const & absentCountries);
  void OnBuildRouteReady(routing::Route const & route, routing::IRouter::ResultCode code);
  void OnRebuildRouteReady(routing::Route const & route, routing::IRouter::ResultCode code);
  void OnRoutePointPassed(RouteMarkType type, int8_t intermediateIndex);
  void OnLocationUpdate(location::GpsInfo & info);
  void SetAllowSendingPoints(bool isAllowed)
  {
    m_trackingReporter.SetAllowSendingPoints(isAllowed);
  }

  void SetTurnNotificationsUnits(measurement_utils::Units const units)
  {
    m_routingSession.SetTurnNotificationsUnits(units);
  }
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine, bool is3dAllowed);
  /// \returns true if altitude information along |m_route| is available and
  /// false otherwise.
  bool HasRouteAltitude() const;

  /// \brief Generates 4 bytes per point image (RGBA) and put the data to |imageRGBAData|.
  /// \param width is width of chart shall be generated in pixels.
  /// \param height is height of chart shall be generated in pixels.
  /// \param imageRGBAData is bits of result image in RGBA.
  /// \param minRouteAltitude is min altitude along the route in altitudeUnits.
  /// \param maxRouteAltitude is max altitude along the route in altitudeUnits.
  /// \param altitudeUnits is units (meters or feet) which is used to pass min and max altitudes.
  /// \returns If there is valid route info and the chart was generated returns true
  /// and false otherwise. If the method returns true it is guaranteed that the size of
  /// |imageRGBAData| is not zero.
  /// \note If HasRouteAltitude() method returns true, GenerateRouteAltitudeChart(...)
  /// could return false if route was deleted or rebuilt between the calls.
  bool GenerateRouteAltitudeChart(uint32_t width, uint32_t height, std::vector<uint8_t> & imageRGBAData,
                                  int32_t & minRouteAltitude, int32_t & maxRouteAltitude,
                                  measurement_utils::Units & altitudeUnits) const;

private:
  void InsertRoute(routing::Route const & route);
  bool IsTrackingReporterEnabled() const;
  void MatchLocationToRoute(location::GpsInfo & info,
                            location::RouteMatchingInfo & routeMatchingInfo) const;

  RouteBuildingCallback m_routingCallback = nullptr;
  Callbacks m_callbacks;
  ref_ptr<df::DrapeEngine> m_drapeEngine = nullptr;
  routing::RouterType m_currentRouterType = routing::RouterType::Count;
  routing::RoutingSession m_routingSession;
  Delegate & m_delegate;
  tracking::Reporter m_trackingReporter;
  BookmarkManager * m_bmManager = nullptr;

  std::vector<dp::DrapeID> m_drapeSubroutes;
  std::mutex m_drapeSubroutesMutex;

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
