#pragma once

#include <jni.h>

#include "map/framework.hpp"

#include "search/result.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "drape/pointers.hpp"
#include "drape/oglcontextfactory.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"

#include "geometry/avg_vector.hpp"

#include "base/timer.hpp"

#include "indexer/map_style.hpp"

#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/cstdint.hpp"

namespace android
{
  class Framework : public storage::CountryTree::CountryTreeListener,
                    public storage::ActiveMapsLayout::ActiveMapsListener
  {
  private:
    drape_ptr<dp::ThreadSafeFactory> m_contextFactory;
    ::Framework m_work;

    typedef shared_ptr<jobject> TJobject;
    TJobject m_javaCountryListener;
    typedef map<int, TJobject> TListenerMap;
    TListenerMap m_javaActiveMapListeners;
    int m_currentSlotID;

    int m_activeMapsConnectionID;

    math::LowPassVector<float, 3> m_sensors[2];
    double m_lastCompass;

    string m_searchQuery;

    map<gui::EWidget, gui::Position> m_guiPositions;

    void MyPositionModeChanged(location::EMyPositionMode mode);

    location::TMyPositionModeChanged m_myPositionModeSignal;
    location::EMyPositionMode m_currentMode;
    bool m_isCurrentModeInitialized;

  public:
    Framework();
    ~Framework();

    storage::Storage & Storage();

    void ShowCountry(storage::TIndex const & idx, bool zoomToDownloadButton);
    storage::TStatus GetCountryStatus(storage::TIndex const & idx) const;

    void OnLocationError(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(location::GpsInfo const & info);
    void OnCompassUpdated(location::CompassInfo const & info, bool forceRedraw);
    void UpdateCompassSensor(int ind, float * arr);

    void Invalidate();

    bool CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi);
    void DeleteDrapeEngine();
    bool IsDrapeEngineCreated();

    void DetachSurface();
    void AttachSurface(JNIEnv * env, jobject jSurface);

    void SetMapStyle(MapStyle mapStyle);
    MapStyle GetMapStyle() const;

    void SetupMeasurementSystem();

    void SetRouter(routing::RouterType type) { m_work.SetRouter(type); }
    routing::RouterType GetRouter() const { return m_work.GetRouter(); }
    routing::RouterType GetLastUsedRouter() const { return m_work.GetLastUsedRouter(); }

    void Resize(int w, int h);

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

    /// Show rect from another activity. Ensure that no LoadState will be called,
    /// when main map activity will become active.
    void ShowSearchResult(search::Result const & r);
    void ShowAllSearchResults(search::Results const & results);

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
    void SetMyPositionMode(location::EMyPositionMode mode);

    void Save3dMode(bool allow3d, bool allow3dBuildings);
    void Set3dMode(bool allow3d, bool allow3dBuildings);
    void Get3dMode(bool & allow3d, bool & allow3dBuildings);

    void SetupWidget(gui::EWidget widget, float x, float y, dp::Anchor anchor);
    void ApplyWidgets();
    void CleanWidgets();

    // Fills mapobject's metadata from UserMark
    void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, UserMark const * userMark);

    using TDrapeTask = function<void()>;
    // Posts a task which must be executed when Drape Engine is alive.
    void PostDrapeTask(TDrapeTask && task);

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

  private:
    vector<TDrapeTask> m_drapeTasksQueue;
    mutex m_drapeQueueMutex;

    // This method must be executed under mutex m_drapeQueueMutex.
    void ExecuteDrapeTasks();
  };
}

extern android::Framework * g_framework;
