#pragma once

#include "map/catalog_headers_provider.hpp"
#include "map/guides_on_map_delegate.hpp"

#include "partners_api/guides_on_map_api.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class GuidesManager final
{
public:
  enum class GuidesState
  {
    // Layer is disabled.
    Disabled,
    // Layer is enabled and no errors after last guide request are found.
    Enabled,
    // No data into requested rect.
    NoData,
    // Attempt to request guides is failed.
    NetworkError,
    // Several attempts to request guides are failed.
    FatalNetworkError,
  };

  class Delegate
  {
  public:
    virtual ~Delegate() = default;
    virtual bool IsGuideDownloaded(std::string const & guideId) const = 0;
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
      std::string m_subTitle;
      Type m_type = Type::City;
      bool m_downloaded = false;
      CityParams m_cityParams;
      OutdoorParams m_outdoorsParams;
    };

    std::vector<Item> m_items;
  };

  GuidesState GetState() const;

  using GuidesStateChangedFn = std::function<void(GuidesState state)>;
  void SetStateListener(GuidesStateChangedFn const & onStateChangedFn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();
  void Reconnect();

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  GuidesGallery GetGallery() const;
  std::string GetActiveGuide() const;
  void SetActiveGuide(std::string const & guideId);

  using GuidesGalleryChangedFn = std::function<void(bool reloadGallery)>;
  void SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChangedFn);

  void SetDelegate(std::unique_ptr<Delegate> delegate);
  void SetApiDelegate(std::unique_ptr<guides_on_map::Api::Delegate> apiDelegate);

private:
  void ChangeState(GuidesState newState);
  void RequestGuides(m2::AnyRectD const & rect, int zoom);
  void Clear();

  GuidesState m_state = GuidesState::Disabled;
  GuidesStateChangedFn m_onStateChanged;
  GuidesGalleryChangedFn m_onGalleryChanged;

  int m_zoom = 0;
  m2::AnyRectD m_currentRect;

  uint64_t m_requestCounter = 0;
  uint8_t m_errorRequestsCount = 0;

  guides_on_map::Api m_api;
  guides_on_map::GuidesOnMap m_guides;
  // Initial value is dummy for debug only.
  std::string m_activeGuide = "048f4c49-ee80-463f-8513-e57ade2303ee";

  std::unique_ptr<Delegate> m_delegate;
};

std::string DebugPrint(GuidesManager::GuidesState state);
