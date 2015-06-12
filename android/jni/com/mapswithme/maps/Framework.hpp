#pragma once

#include <jni.h>

#include "map/framework.hpp"

#include "search/result.hpp"

#include "drape_gui/skin.hpp"

#include "drape/pointers.hpp"
#include "drape/oglcontextfactory.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"

#include "geometry/avg_vector.hpp"

#include "base/deferred_task.hpp"
#include "base/timer.hpp"

#include "indexer/map_style.hpp"

#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"

namespace android
{
  class Framework : public storage::CountryTree::CountryTreeListener,
                    public storage::ActiveMapsLayout::ActiveMapsListener
  {
  private:
    drape_ptr<dp::ThreadSafeFactory> m_contextFactory;
    ::Framework m_work;

    unique_ptr<gui::Skin> m_skin;

    typedef shared_ptr<jobject> TJobject;

    TJobject m_javaCountryListener;
    typedef map<int, TJobject> TListenerMap;
    TListenerMap m_javaActiveMapListeners;
    int m_currentSlotID;

    int m_activeMapsConnectionID;
    bool m_doLoadState;

    math::LowPassVector<float, 3> m_sensors[2];
    double m_lastCompass;

    unique_ptr<DeferredTask> m_deferredTask;
    bool m_wasLongClick;

    int m_densityDpi;
    int m_screenWidth;
    int m_screenHeight;

    string m_searchQuery;

    float GetBestDensity(int densityDpi);

    void MyPositionModeChanged(location::EMyPositionMode mode);

    location::TMyPositionModeChanged m_myPositionModeSignal;
    location::EMyPositionMode m_currentMode;

  public:
    Framework();
    ~Framework();

    storage::Storage & Storage();

    void DontLoadState() { m_doLoadState = false; }

    void ShowCountry(storage::TIndex const & idx, bool zoomToDownloadButton);
    storage::TStatus GetCountryStatus(storage::TIndex const & idx) const;

    void OnLocationError(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(location::GpsInfo const & info);
    void OnCompassUpdated(location::CompassInfo const & info, bool force);
    void UpdateCompassSensor(int ind, float * arr);

    void Invalidate();

    bool CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi);
    void DeleteDrapeEngine();

    void SetMapStyle(MapStyle mapStyle);
    MapStyle GetMapStyle() const;

    void SetRouter(routing::RouterType type) { m_work.SetRouter(type); }
    routing::RouterType GetRouter() const { return m_work.GetRouter(); }
    routing::RouterType GetLastUsedRouter() const { return m_work.GetLastUsedRouter(); }

    void Resize(int w, int h);
    void Touch(int action, int mask, double x1, double y1, double x2, double y2);

    /// Show rect from another activity. Ensure that no LoadState will be called,
    /// when main map activity will become active.
    void ShowSearchResult(search::Result const & r);
    void ShowAllSearchResults();

    bool Search(search::SearchParams const & params);
    string GetLastSearchQuery() { return m_searchQuery; }
    void ClearLastSearchQuery() { m_searchQuery.clear(); }

    void LoadState();
    void SaveState();

    void AddLocalMaps();
    void RemoveLocalMaps();

    storage::TIndex GetCountryIndex(double lat, double lon) const;
    string GetCountryCode(double lat, double lon) const;

    string GetCountryNameIfAbsent(m2::PointD const & pt) const;
    m2::PointD GetViewportCenter() const;

    void AddString(string const & name, string const & value);

    void Scale(::Framework::EScaleMode mode);

    BookmarkAndCategory AddBookmark(size_t category, m2::PointD const & pt, BookmarkData & bm);
    void ReplaceBookmark(BookmarkAndCategory const & ind, BookmarkData & bm);
    size_t ChangeBookmarkCategory(BookmarkAndCategory const & ind, size_t newCat);

    ::Framework * NativeFramework();

    bool IsDownloadingActive();

    bool ShowMapForURL(string const & url);

    void DeactivatePopup();

    string GetOutdatedCountriesString();

    void ShowTrack(int category, int track);

    void SetCountryTreeListener(shared_ptr<jobject> objPtr);
    void ResetCountryTreeListener();

    int AddActiveMapsListener(shared_ptr<jobject> obj);
    void RemoveActiveMapsListener(int slotID);

    void SetMyPositionModeListener(location::TMyPositionModeChanged const & fn);
    location::EMyPositionMode GetMyPositionMode() const;

    // Fills mapobject's metadata from UserMark
    void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, UserMark const * userMark);

  public:
    virtual void ItemStatusChanged(int childPosition);
    virtual void ItemProgressChanged(int childPosition, storage::LocalAndRemoteSizeT const & sizes);

    virtual void CountryGroupChanged(storage::ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                     storage::ActiveMapsLayout::TGroup const & newGroup, int newPosition);
    virtual void CountryStatusChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                                      storage::TStatus const & oldStatus, storage::TStatus const & newStatus);
    virtual void CountryOptionsChanged(storage::ActiveMapsLayout::TGroup const & group,
                                       int position, MapOptions const & oldOpt,
                                       MapOptions const & newOpt);
    virtual void DownloadingProgressUpdate(storage::ActiveMapsLayout::TGroup const & group, int position,
                                           storage::LocalAndRemoteSizeT const & progress);
  };
}

extern android::Framework * g_framework;
