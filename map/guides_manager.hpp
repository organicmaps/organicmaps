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
    Disabled,
    Enabled,
    NoData,
    NetworkError
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

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  GuidesGallery GetGallery() const;
  std::string GetActiveGuide() const;
  void SetActiveGuide(std::string const & guideId);

  using GuidesGalleryChangedFn = std::function<void(bool reloadGallery)>;
  void SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChangedFn);

  void SetApiDelegate(std::unique_ptr<GuidesOnMapDelegate> apiDelegate);

private:
  void ChangeState(GuidesState newState);

  GuidesState m_state = GuidesState::Disabled;
  GuidesStateChangedFn m_onStateChangedFn;
  GuidesGalleryChangedFn m_onGalleryChangedFn;

  guides_on_map::Api m_api;
};

std::string DebugPrint(GuidesManager::GuidesState state);
