#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"

#include "../../../../../search/result.hpp"

#include "../../../../../geometry/avg_vector.hpp"

#include "../../../../../base/timer.hpp"
#include "../../../../../base/scheduled_task.hpp"
#include "../../../../../base/strings_bundle.hpp"

#include "../../../../../std/shared_ptr.hpp"

#include "../../../nv_event/nv_event.hpp"


class CountryStatusDisplay;

namespace android
{
  class Framework
  {
  private:
    ::Framework m_work;
    VideoTimer * m_videoTimer;

    shared_ptr<jobject> m_javaCountryListenerPtr;
    shared_ptr<jobject> m_javaActiveCountryListenerPtr;
    storage::CountryTree::CountryTreeListener * m_treeListener;
    storage::ActiveMapsLayout::ActiveMapsListener * m_activeMapsListener;

    void CallRepaint();

    double m_x1;
    double m_y1;
    double m_x2;
    double m_y2;
    int m_mask;

    bool m_doLoadState;

    /// @name Single click processing parameters.
    //@{
    my::Timer m_doubleClickTimer;
    bool m_isCleanSingleClick;
    double m_lastX1;
    double m_lastY1;
    //@}

    math::LowPassVector<float, 3> m_sensors[2];
    double m_lastCompass;

    unique_ptr<ScheduledTask> m_scheduledTask;
    bool m_wasLongClick;

    void StartTouchTask(double x, double y, unsigned ms);
    bool KillTouchTask();
    void OnProcessTouchTask(double x, double y, unsigned ms);

    string m_searchQuery;

    void SetBestDensity(int densityDpi, RenderPolicy::Params & params);

  public:
    Framework();
    ~Framework();

    storage::Storage & Storage();
    CountryStatusDisplay * GetCountryStatusDisplay();

    void ShowCountry(storage::TIndex const & idx, bool zoomToDownloadButton);
    storage::TStatus GetCountryStatus(storage::TIndex const & idx) const;

    void OnLocationError(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(location::GpsInfo const & info);
    void OnCompassUpdated(location::CompassInfo const & info);
    void UpdateCompassSensor(int ind, float * arr);

    void Invalidate();

    bool InitRenderPolicy(int densityDpi, int screenWidth, int screenHeight);
    void DeleteRenderPolicy();

    void Resize(int w, int h);

    void DrawFrame();

    void Move(int mode, double x, double y);
    void Zoom(int mode, double x1, double y1, double x2, double y2);
    void Touch(int action, int mask, double x1, double y1, double x2, double y2);

    /// Show rect from another activity. Ensure that no LoadState will be called,
    /// when main map activity will become active.
    void ShowSearchResult(search::Result const & r);
    void ShowAllSearchResults();

    bool Search(search::SearchParams const & params);
    string GetLastSearchQuery() { return m_searchQuery; }
    void ClearLastSearchQuery() { m_searchQuery.clear(); }
    //void CleanSearchLayerOnMap();

    void LoadState();
    void SaveState();

    void SetupMeasurementSystem();

    void AddLocalMaps();
    void RemoveLocalMaps();

    void GetMapsWithoutSearch(vector<string> & out) const;

    storage::TIndex GetCountryIndex(double lat, double lon) const;
    string GetCountryCode(double lat, double lon) const;

    string GetCountryNameIfAbsent(m2::PointD const & pt) const;
    m2::PointD GetViewportCenter() const;

    void AddString(string const & name, string const & value);

    void Scale(double k);

    BookmarkAndCategory AddBookmark(size_t category, m2::PointD const & pt, BookmarkData & bm);
    void ReplaceBookmark(BookmarkAndCategory const & ind, BookmarkData & bm);
    size_t ChangeBookmarkCategory(BookmarkAndCategory const & ind, size_t newCat);

    ::Framework * NativeFramework();
    PinClickManager & GetPinClickManager() { return m_work.GetBalloonManager(); }

    bool IsDownloadingActive();

    bool ShowMapForURL(string const & url);

    void DeactivatePopup();

    string GetOutdatedCountriesString();

    void ShowTrack(int category, int track);

    storage::CountryTree::CountryTreeListener * setCountryTreeListener(shared_ptr<jobject> objPtr);

    void resetCountryTreeListener();

    storage::ActiveMapsLayout::ActiveMapsListener * setActiveMapsListener(shared_ptr<jobject> objPtr);

    void resetActiveMapsListener();

    shared_ptr<jobject> getJavaCountryListener();

    shared_ptr<jobject> getJavaActiveCountryListener();
  };
}

extern android::Framework * g_framework;

namespace guides
{
  jobject GuideNativeToJava(JNIEnv *, GuideInfo const &);
}
