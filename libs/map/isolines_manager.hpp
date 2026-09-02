#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <functional>
#include <optional>
#include <string>

// The dynamic terrain isolines layer switch and its availability hint: NoData means the
// viewport misses the downloaded terrain coverage, and the platforms show the neutral
// availability hint from isolines_location_error_dialog.
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

  // Availability of the downloaded TWM terrain for a rect.
  using HasTerrainFn = std::function<bool(m2::RectD const &)>;
  void SetHasTerrainFn(HasTerrainFn const & fn) { m_hasTerrainFn = fn; }

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

private:
  void ChangeState(IsolinesState newState);

  IsolinesState m_state = IsolinesState::Disabled;
  IsolinesStateChangedFn m_onStateChangedFn;

  HasTerrainFn m_hasTerrainFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;
};

std::string DebugPrint(IsolinesManager::IsolinesState state);
