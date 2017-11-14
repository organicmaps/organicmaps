#pragma once

#include "map/bookmark_manager.hpp"
#include "map/routing_mark.hpp"
#include "map/transit_reader.hpp"

#include "routing/route.hpp"
#include "routing/routing_session.hpp"

#include "storage/index.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "tracking/reporter.hpp"

#include "geometry/point2d.hpp"

#include "base/thread_checker.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

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
  size_t m_intermediateIndex = 0;
  bool m_isPassed = false;
  bool m_isMyPosition = false;
  m2::PointD m_position;
};

enum class TransitType: uint32_t
{
  // Do not change the order!
  Pedestrian,
  Subway
};

struct TransitStepInfo
{
  TransitStepInfo() = default;
  TransitStepInfo(TransitType type, double distance, double time,
                  std::string const & number = "", uint32_t color = 0);

  bool IsEqualType(TransitStepInfo const & ts) const;

  TransitType m_type = TransitType::Pedestrian;
  double m_distance = 0.0;
  double m_time = 0.0;
  std::string m_number;
  uint32_t m_color;
};

struct TransitRouteInfo
{
  double m_totalDistance = 0.0;
  double m_totalTime = 0.0;
  double m_totalPedestrianDistance = 0.0;
  double m_totalPedestrianTime = 0.0;
  std::vector<TransitStepInfo> m_steps;

  void AddStep(TransitStepInfo const & step);
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
    using IndexGetterFn = std::function<Index &()>;
    using CountryInfoGetterFn = std::function<storage::CountryInfoGetter &()>;
    using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;
    using FeatureCallback = std::function<void (FeatureType const &)>;
    using ReadFeaturesFn = std::function<void (FeatureCallback const &, std::vector<FeatureID> const &)>;


    template <typename IndexGetter, typename CountryInfoGetter, typename CountryParentNameGetter, typename FeatureReader>
    Callbacks(IndexGetter && featureIndexGetter, CountryInfoGetter && countryInfoGetter,
              CountryParentNameGetter && countryParentNameGetter, FeatureReader && readFeatures)
      : m_indexGetter(std::forward<IndexGetter>(featureIndexGetter))
      , m_countryInfoGetter(std::forward<CountryInfoGetter>(countryInfoGetter))
      , m_countryParentNameGetterFn(std::forward<CountryParentNameGetter>(countryParentNameGetter))
      , m_readFeaturesFn(std::forward<FeatureReader>(readFeatures))
    {}

    IndexGetterFn m_indexGetter;
    CountryInfoGetterFn m_countryInfoGetter;
    CountryParentNameGetterFn m_countryParentNameGetterFn;
    TReadFeaturesFn m_readFeaturesFn;
  };

  using RouteBuildingCallback =
      std::function<void(routing::IRouter::ResultCode, storage::TCountriesVec const &)>;
  using RouteProgressCallback = std::function<void(float)>;

  enum class Recommendation
  {
    // It can be recommended if location is found almost immediately
    // after restoring route points from file. In this case we can
    // rebuild route using "my position".
    RebuildAfterPointsLoading = 0,
  };
  using RouteRecommendCallback = std::function<void(Recommendation)>;

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

  void SetRouteBuildingListener(RouteBuildingCallback const & buildingCallback)
  {
    m_routingCallback = buildingCallback;
  }
  /// See warning above.
  void SetRouteProgressListener(RouteProgressCallback const & progressCallback)
  {
    m_routingSession.SetProgressCallback(progressCallback);
  }
  void SetRouteRecommendationListener(RouteRecommendCallback const & recommendCallback)
  {
    m_routeRecommendCallback = recommendCallback;
  }
  void FollowRoute();
  void CloseRouting(bool removeRoutePoints);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const
  {
    m_routingSession.GetRouteFollowingInfo(info);
  }

  TransitRouteInfo GetTransitRouteInfo() const;

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
  void RemoveRoutePoint(RouteMarkType type, size_t intermediateIndex = 0);
  void RemoveRoutePoints();
  void RemoveIntermediateRoutePoints();
  void MoveRoutePoint(size_t currentIndex, size_t targetIndex);
  void MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                      RouteMarkType targetType, size_t targetIntermediateIndex);
  void HideRoutePoint(RouteMarkType type, size_t intermediateIndex = 0);
  bool CouldAddIntermediatePoint() const;
  bool IsMyPosition(RouteMarkType type, size_t intermediateIndex = 0);

  void SetRouterImpl(routing::RouterType type);
  void RemoveRoute(bool deactivateFollowing);

  void CheckLocationForRouting(location::GpsInfo const & info);
  void CallRouteBuilded(routing::IRouter::ResultCode code,
                        storage::TCountriesVec const & absentCountries);
  void OnBuildRouteReady(routing::Route const & route, routing::IRouter::ResultCode code);
  void OnRebuildRouteReady(routing::Route const & route, routing::IRouter::ResultCode code);
  void OnRoutePointPassed(RouteMarkType type, size_t intermediateIndex);
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

  uint32_t OpenRoutePointsTransaction();
  void ApplyRoutePointsTransaction(uint32_t transactionId);
  void CancelRoutePointsTransaction(uint32_t transactionId);
  static uint32_t InvalidRoutePointsTransactionId();

  /// \returns true if there are route points saved in file and false otherwise.
  bool HasSavedRoutePoints() const;
  /// \brief It loads road points from file and delete file after loading.
  /// \returns true if route points loaded and false otherwise.
  bool LoadRoutePoints();
  /// \brief It saves route points to file.
  void SaveRoutePoints() const;
  /// \brief It deletes file with saved route points if it exists.
  void DeleteSavedRoutePoints();

  void UpdatePreviewMode();
  void CancelPreviewMode();

private:
  void InsertRoute(routing::Route const & route);
  bool IsTrackingReporterEnabled() const;
  void MatchLocationToRoute(location::GpsInfo & info,
                            location::RouteMatchingInfo & routeMatchingInfo) const;
  location::RouteMatchingInfo GetRouteMatchingInfo(location::GpsInfo & info);
  uint32_t GenerateRoutePointsTransactionId() const;

  void SetPointsFollowingMode(bool enabled);

  void ReorderIntermediatePoints();

  m2::RectD ShowPreviewSegments(std::vector<RouteMarkData> const & routePoints);
  void HidePreviewSegments();

  std::vector<dp::DrapeID> GetSubrouteIds() const;
  void SetSubroutesVisibility(bool visible);

  void CancelRecommendation(Recommendation recommendation);

  RouteBuildingCallback m_routingCallback = nullptr;
  RouteRecommendCallback m_routeRecommendCallback = nullptr;
  Callbacks m_callbacks;
  df::DrapeEngineSafePtr m_drapeEngine;
  routing::RouterType m_currentRouterType = routing::RouterType::Count;
  bool m_loadAltitudes = false;
  routing::RoutingSession m_routingSession;
  Delegate & m_delegate;
  tracking::Reporter m_trackingReporter;
  BookmarkManager * m_bmManager = nullptr;

  std::vector<dp::DrapeID> m_drapeSubroutes;
  mutable std::mutex m_drapeSubroutesMutex;

  std::unique_ptr<location::GpsInfo> m_gpsInfoCache;

  TransitRouteInfo m_transitRouteInfo;

  struct RoutePointsTransaction
  {
    std::vector<RouteMarkData> m_routeMarks;
  };
  std::map<uint32_t, RoutePointsTransaction> m_routePointsTransactions;
  std::chrono::steady_clock::time_point m_loadRoutePointsTimestamp;

  TransitReadManager m_transitReadManager;

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
