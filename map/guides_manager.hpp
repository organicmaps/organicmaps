#pragma once

#include "map/bookmark_manager.hpp"
#include "map/catalog_headers_provider.hpp"
#include "map/guides_marks.hpp"
#include "map/guides_on_map_delegate.hpp"
#include "map/layers_statistics.hpp"

#include "partners_api/guides_on_map_api.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/task_loop.hpp"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

class GuidesManager final
{
public:
  enum class GuidesState
  {
    // Layer is disabled.
    Disabled,
    // Layer is enabled and no errors after last guide request are found.
    Enabled,
    // Same as Enabled + nonempty guides data has been received and is available.
    HasData,
    // Same as Enabled + no data received for the requested rect.
    NoData,
    // Attempt to request guides is failed.
    NetworkError,
    // Several attempts to request guides are failed.
    FatalNetworkError,
  };

  struct GuidesGallery
  {
    struct Item
    {
      struct CityParams
      {
        int m_bookmarksCount = 0;
        bool m_trackIsAvailable = false;
      };

      struct OutdoorParams
      {
        std::string m_tag;
        // Distance in meters.
        double m_distance = 0.0;
        // Duration in seconds.
        uint32_t m_duration = 0;
        // Ascent in meters.
        uint16_t m_ascent = 0;
      };

      enum class Type
      {
        City,
        Outdoor
      };

      std::string m_guideId;
      std::string m_url;
      std::string m_imageUrl;
      std::string m_title;
      Type m_type = Type::City;
      bool m_downloaded = false;
      CityParams m_cityParams;
      OutdoorParams m_outdoorsParams;
    };

    std::deque<Item> m_items;
  };

  using CloseGalleryFn = std::function<void()>;
  explicit GuidesManager(CloseGalleryFn && closeGalleryFn);

  GuidesState GetState() const;

  using GuidesStateChangedFn = std::function<void(GuidesState state)>;
  void SetStateListener(GuidesStateChangedFn const & onStateChangedFn);

  void UpdateViewport(ScreenBase const & screen);
  void Reconnect();

  void SetEnabled(bool enabled, bool silentMode = false, bool suggestZoom = true);
  bool IsEnabled() const;

  GuidesGallery GetGallery() const;
  std::string GetActiveGuide() const;
  void SetActiveGuide(std::string const & guideId);
  void ResetActiveGuide();

  uint64_t GetShownGuidesCount() const;

  using GuidesGalleryChangedFn = std::function<void(bool reloadGallery)>;
  void SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChangedFn);

  void SetBookmarkManager(BookmarkManager * bmManager);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetApiDelegate(std::unique_ptr<guides_on_map::Api::Delegate> apiDelegate);

  void OnClusterSelected(GuidesClusterMark const & mark, ScreenBase const & screen);
  void OnGuideSelected();

  void LogGuideSelectedStatistic();

private:
  void ChangeState(GuidesState newState, bool force = false, bool needNotify = true);
  void RequestGuides(bool suggestZoom = false);
  void Clear();

  bool IsGuideDownloaded(std::string const & guideId) const;
  void UpdateDownloadedStatus();
  void UpdateGuidesMarks();
  void UpdateActiveGuide();

  bool IsRequestParamsInitialized() const;

  void TrackStatistics() const;

  GuidesGallery::Item MakeGalleryItem(guides_on_map::GuidesNode const & guide) const;

  void OnRequestSucceed(guides_on_map::GuidesOnMap const & guides, bool suggestZoom,
                        uint64_t requestNumber);
  void OnRequestError();

  CloseGalleryFn m_closeGallery;

  GuidesState m_state = GuidesState::Disabled;
  GuidesStateChangedFn m_onStateChanged;
  GuidesGalleryChangedFn m_onGalleryChanged;

  m2::AnyRectD m_lastShownViewport;
  int m_zoom = 0;
  ScreenBase m_screen;

  uint64_t m_requestCounter = 0;
  uint8_t m_errorRequestsCount = 0;
  base::TaskLoop::TaskId m_retryAfterErrorRequestId = base::TaskLoop::kNoId;
  base::TaskLoop::TaskId m_previousRequestsId = base::TaskLoop::kNoId;

  guides_on_map::Api m_api;
  guides_on_map::GuidesOnMap m_guides;
  std::string m_activeGuide;

  BookmarkManager * m_bmManager = nullptr;
  df::DrapeEngineSafePtr m_drapeEngine;

  std::unordered_set<std::string> m_shownGuides;
  LayersStatistics m_statistics;

  bool m_silentMode = false;
};

std::string DebugPrint(GuidesManager::GuidesState state);
