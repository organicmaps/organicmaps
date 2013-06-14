#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"
#include "../../../../../map/bookmark_balloon.hpp"

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

    math::AvgVector<float, 3> m_sensors[2];

    shared_ptr<ScheduledTask> m_scheduledTask;
    bool m_wasLongClick;

    void StartTouchTask(double x, double y, unsigned ms);
    void KillTouchTask();
    void OnProcessTouchTask(double x, double y, unsigned ms);

    string m_searchQuery;

    void SetBestDensity(int densityDpi, RenderPolicy::Params & params);

  public:
    Framework();
    ~Framework();

    storage::Storage & Storage();
    CountryStatusDisplay * GetCountryStatusDisplay();

    void ShowCountry(storage::TIndex const & idx);
    storage::TStatus GetCountryStatus(storage::TIndex const & idx) const;
    void DeleteCountry(storage::TIndex const & idx);

    void OnLocationError(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(uint64_t time, double lat, double lon, float accuracy);
    void OnCompassUpdated(uint64_t time, double magneticNorth, double trueNorth, double accuracy);
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
    /// when maim map activity will become active.
    void ShowSearchResult(search::Result const & r);

    bool Search(search::SearchParams const & params);
    string GetLastSearchQuery() { return m_searchQuery; }
    void ClearLastSearchQuery() { m_searchQuery.clear(); }

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

    BookmarkAndCategory AddBookmark(size_t category, Bookmark & bm);
    void ReplaceBookmark(BookmarkAndCategory const & ind, Bookmark & bm);
    size_t ChangeBookmarkCategory(BookmarkAndCategory const & ind, size_t newCat);

    ::Framework * NativeFramework();
    BalloonManager & GetBalloonManager() { return m_work.GetBalloonManager(); }

    bool IsDownloadingActive();

    bool SetViewportByUrl(string const & ulr);

    void DeactivatePopup();
  };
}

extern android::Framework * g_framework;
