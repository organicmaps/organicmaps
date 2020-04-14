#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <functional>
#include <string>
#include <vector>

#include <boost/optional.hpp>

class GuidesManager final
{
public:
  enum class GuidesState
  {
    Disabled,
    Enabled,
    NoData,
    NoInternet,
    ServerError
  };

  GuidesState GetState() const;

  using GuidesStateChangedFn = std::function<void(GuidesState)>;
  void SetStateListener(GuidesStateChangedFn const & onStateChangedFn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  struct GuideInfo
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
    std::string m_imageUrl;
    std::string m_title;
    std::string m_subTitle;
    Type m_type = Type::City;
    boost::optional<CityParams> m_cityParams;
    boost::optional<OutdoorParams> m_outdoorsParams;
  };

  bool GetGallery(std::string const & guideId, std::vector<GuideInfo> & guidesInfo);
  void SetActiveGuide(std::string const & guideId);

private:
  GuidesState m_state = GuidesState::Disabled;
  GuidesStateChangedFn m_onStateChangedFn;
};

std::string DebugPrint(GuidesManager::GuidesState state);
