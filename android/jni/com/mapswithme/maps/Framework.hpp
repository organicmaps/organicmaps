#pragma once

#include <jni.h>

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/power_management/power_manager.hpp"

#include "ugc/api.hpp"

#include "search/result.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "drape/pointers.hpp"
#include "drape/graphics_context_factory.hpp"

#include "local_ads/event.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/locals_api.hpp"
#include "partners_api/promo_api.hpp"
#include "partners_api/utm.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/map_style.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"

#include "geometry/avg_vector.hpp"

#include "base/timer.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

class DataSource;
struct FeatureID;

namespace search
{
struct EverywhereSearchParams;
}

namespace booking
{
struct BlockParams;
}

namespace android
{
  class Framework : private power_management::PowerManager::Subscriber
  {
  private:
    drape_ptr<dp::ThreadSafeFactory> m_oglContextFactory;
    drape_ptr<dp::GraphicsContextFactory> m_vulkanContextFactory;
    ::Framework m_work;

    math::LowPassVector<float, 3> m_sensors[2];
    double m_lastCompass;

    std::string m_searchQuery;

    bool m_isSurfaceDestroyed;

    std::map<gui::EWidget, gui::Position> m_guiPositions;

    void TrafficStateChanged(TrafficManager::TrafficState state);
    void TransitSchemeStateChanged(TransitReadManager::TransitSchemeState state);
    void IsolinesSchemeStateChanged(IsolinesManager::IsolinesState state);
    void GuidesLayerStateChanged(GuidesManager::GuidesState state);

    void MyPositionModeChanged(location::EMyPositionMode mode, bool routingActive);

    location::TMyPositionModeChanged m_myPositionModeSignal;
    location::EMyPositionMode m_currentMode;
    bool m_isCurrentModeInitialized;

    TrafficManager::TrafficStateChangedFn m_onTrafficStateChangedFn;
    TransitReadManager::TransitStateChangedFn m_onTransitStateChangedFn;
    IsolinesManager::IsolinesStateChangedFn m_onIsolinesStateChangedFn;
    GuidesManager::GuidesStateChangedFn m_onGuidesStateChangedFn;

    bool m_isChoosePositionMode;

  public:
    Framework();

    storage::Storage & GetStorage();
    DataSource const & GetDataSource();

    void ShowNode(storage::CountryId const & countryId, bool zoomToDownloadButton);

    void OnLocationError(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(location::GpsInfo const & info);
    void OnCompassUpdated(location::CompassInfo const & info, bool forceRedraw);

    bool CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi, bool firstLaunch,
                           bool launchByDeepLink, int appVersionCode);
    bool IsDrapeEngineCreated();
    bool DestroySurfaceOnDetach();
    void DetachSurface(bool destroySurface);
    bool AttachSurface(JNIEnv * env, jobject jSurface);
    void PauseSurfaceRendering();
    void ResumeSurfaceRendering();

    void SetMapStyle(MapStyle mapStyle);
    void MarkMapStyle(MapStyle mapStyle);
    MapStyle GetMapStyle() const;

    void SetupMeasurementSystem();

    RoutingManager & GetRoutingManager() { return m_work.GetRoutingManager(); }
    void SetRouter(routing::RouterType type) { m_work.GetRoutingManager().SetRouter(type); }
    routing::RouterType GetRouter() const { return m_work.GetRoutingManager().GetRouter(); }
    routing::RouterType GetLastUsedRouter() const
    {
      return m_work.GetRoutingManager().GetLastUsedRouter();
    }

    void Resize(JNIEnv * env, jobject jSurface, int w, int h);

    struct Finger
    {
      Finger(int64_t id, float x, float y)
        : m_id(id)
        , m_x(x)
        , m_y(y)
      {
      }

      int64_t m_id;
      float m_x, m_y;
    };

    void Touch(int action, Finger const & f1, Finger const & f2, uint8_t maskedPointer);

    bool Search(search::EverywhereSearchParams const & params);
    std::string GetLastSearchQuery() { return m_searchQuery; }
    void ClearLastSearchQuery() { m_searchQuery.clear(); }

    void AddLocalMaps();
    void RemoveLocalMaps();

    m2::PointD GetViewportCenter() const;

    void AddString(std::string const & name, std::string const & value);

    void Scale(::Framework::EScaleMode mode);
    void Scale(m2::PointD const & centerPt, int targetZoom, bool animate);

    void ReplaceBookmark(kml::MarkId markId, kml::BookmarkData & bm);
    void MoveBookmark(kml::MarkId markId, kml::MarkGroupId curCat, kml::MarkGroupId newCat);

    ::Framework * NativeFramework();

    bool IsDownloadingActive();

    bool ShowMapForURL(std::string const & url);

    void DeactivatePopup();

    std::string GetOutdatedCountriesString();

    void SetMyPositionModeListener(location::TMyPositionModeChanged const & fn);
    location::EMyPositionMode GetMyPositionMode();
    void OnMyPositionModeChanged(location::EMyPositionMode mode);
    void SwitchMyPositionNextMode();

    void SetTrafficStateListener(TrafficManager::TrafficStateChangedFn const & fn);
    void SetTransitSchemeListener(TransitReadManager::TransitStateChangedFn const & fn);
    void SetIsolinesListener(IsolinesManager::IsolinesStateChangedFn const & fn);
    void SetGuidesListener(GuidesManager::GuidesStateChangedFn const & fn);

    bool IsTrafficEnabled();
    void EnableTraffic();
    void DisableTraffic();

    void Save3dMode(bool allow3d, bool allow3dBuildings);
    void Set3dMode(bool allow3d, bool allow3dBuildings);
    void Get3dMode(bool & allow3d, bool & allow3dBuildings);

    void SetChoosePositionMode(bool isChoosePositionMode, bool isBusiness, bool hasPosition, m2::PointD const & position);
    bool GetChoosePositionMode();

    void SetupWidget(gui::EWidget widget, float x, float y, dp::Anchor anchor);
    void ApplyWidgets();
    void CleanWidgets();

    place_page::Info & GetPlacePageInfo();
    void RequestBookingMinPrice(JNIEnv * env, jobject policy, booking::BlockParams && params,
                                booking::BlockAvailabilityCallback const & callback);
    void RequestBookingInfo(JNIEnv * env, jobject policy, 
                            std::string const & hotelId, std::string const & lang,
                            booking::GetHotelInfoCallback const & callback);

    bool IsAutoRetryDownloadFailed();
    bool IsDownloadOn3gEnabled();
    void EnableDownloadOn3g();
    void DisableAdProvider(ads::Banner::Type const type, ads::Banner::Place const place);
    uint64_t RequestTaxiProducts(JNIEnv * env, jobject policy, ms::LatLon const & from,
                                 ms::LatLon const & to, taxi::SuccessCallback const & onSuccess,
                                 taxi::ErrorCallback const & onError);
    taxi::RideRequestLinks GetTaxiLinks(JNIEnv * env, jobject policy, taxi::Provider::Type type,
                                        std::string const & productId, ms::LatLon const & from,
                                        ms::LatLon const & to);
    void RequestUGC(FeatureID const & fid, ugc::Api::UGCCallback const & ugcCallback);
    void SetUGCUpdate(FeatureID const & fid, ugc::UGCUpdate const & ugc,
                      ugc::Api::OnResultCallback const & callback = nullptr);
    void UploadUGC();

    int ToDoAfterUpdate() const;

    uint64_t GetLocals(JNIEnv * env, jobject policy, double lat, double lon,
                       locals::LocalsSuccessCallback const & successFn,
                       locals::LocalsErrorCallback const & errorFn);
    void GetPromoCityGallery(JNIEnv * env, jobject policy, m2::PointD const & point, UTM utm,
                             promo::CityGalleryCallback const & onSuccess,
                             promo::OnError const & onError);
    void GetPromoPoiGallery(JNIEnv * env, jobject policy, m2::PointD const & point,
                            promo::Tags const & tags, bool useCoordinates, UTM utm,
                            promo::CityGalleryCallback const & onSuccess,
                            promo::OnError const & onError);
    promo::AfterBooking GetPromoAfterBooking(JNIEnv * env, jobject policy);
    std::string GetPromoCityUrl(JNIEnv * env, jobject policy, jdouble lat, jdouble lon);

    void LogLocalAdsEvent(local_ads::EventType event, double lat, double lon, uint16_t accuracy);

    // PowerManager::Subscriber overrides:
    void OnPowerFacilityChanged(power_management::Facility const facility, bool enabled) override;
    void OnPowerSchemeChanged(power_management::Scheme const actualScheme) override;

    FeatureID BuildFeatureId(JNIEnv * env, jobject featureId);
  };
}

extern std::unique_ptr<android::Framework> g_framework;
