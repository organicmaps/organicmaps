#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/data_source.hpp"
#include "indexer/isolines_info.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>

class IsolinesManager final
{
public:
  enum class IsolinesState
  {
    Disabled,
    Enabled,
    ExpiredData,
    NoData
  };

  using IsolinesStateChangedFn = std::function<void(IsolinesState)>;
  using GetMwmsByRectFn = std::function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;

  IsolinesManager(DataSource & dataSource, GetMwmsByRectFn const & getMwmsByRectFn);

  IsolinesState GetState() const;
  void SetStateListener(IsolinesStateChangedFn const & onStateChangedFn);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  bool IsVisible() const;

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  isolines::Quality GetDataQuality(MwmSet::MwmId const & id) const;

  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

private:
  enum class Availability
  {
    Available,
    NoData,
    ExpiredData
  };

  struct Info
  {
    Info() = default;
    Info(Availability availability, isolines::Quality quality) : m_availability(availability), m_quality(quality) {}

    Availability m_availability = Availability::NoData;
    isolines::Quality m_quality = isolines::Quality::None;
  };

  void UpdateState();
  void ChangeState(IsolinesState newState);
  Info const & LoadIsolinesInfo(MwmSet::MwmId const & id) const;

  IsolinesState m_state = IsolinesState::Disabled;
  IsolinesStateChangedFn m_onStateChangedFn;

  DataSource & m_dataSource;
  GetMwmsByRectFn m_getMwmsByRectFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;

  std::vector<MwmSet::MwmId> m_lastMwms;
  mutable std::map<MwmSet::MwmId, Info> m_mwmCache;
};

std::string DebugPrint(IsolinesManager::IsolinesState state);
