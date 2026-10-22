#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/screenbase.hpp"

#include <functional>
#include <optional>
#include <string>

// The dynamic terrain isolines layer switch and its availability hint: NoData means an
// installed map in the viewport lacks its downloaded terrain, the platforms prompt the
// user to download it (see isolines_location_error_dialog).
class IsolinesManager final
{
public:
  enum class IsolinesState
  {
    Disabled,
    Enabled,
    NoData
  };

  using IsolinesStateChangedFn = std::function<void(IsolinesState)>;

  IsolinesState GetState() const;
  void SetStateListener(IsolinesStateChangedFn const & onStateChangedFn);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  bool IsVisible() const;

  void UpdateViewport(ScreenBase const & screen);
  // The terrain availability of the current viewport, applied to the enabled visible layer.
  void SetHasTerrain(bool hasTerrain);

private:
  void ChangeState(IsolinesState newState);

  IsolinesState m_state = IsolinesState::Disabled;
  IsolinesStateChangedFn m_onStateChangedFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;
};

std::string DebugPrint(IsolinesManager::IsolinesState state);
