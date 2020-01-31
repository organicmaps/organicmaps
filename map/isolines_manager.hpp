#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>

#include <boost/optional.hpp>

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

  void SetStateListener(IsolinesStateChangedFn const & onStateChangedFn);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

private:
  void UpdateState();
  void ChangeState(IsolinesState newState);

  enum class Availability
  {
    Available,
    NoData,
    ExpiredData
  };

  IsolinesState m_state = IsolinesState::Disabled;
  IsolinesStateChangedFn m_onStateChangedFn;

  DataSource & m_dataSource;
  GetMwmsByRectFn m_getMwmsByRectFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  boost::optional<ScreenBase> m_currentModelView;

  std::vector<MwmSet::MwmId> m_lastMwms;
  std::map<MwmSet::MwmId, Availability> m_mwmCache;
};

std::string DebugPrint(IsolinesManager::IsolinesState state);
