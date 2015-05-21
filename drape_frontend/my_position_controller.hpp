#pragma once

#include "drape_frontend/my_position.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"

#include "base/timer.hpp"

namespace df
{

class MyPositionController
{
public:
  MyPositionController(location::EMyPositionMode initMode);

  m2::PointD const & Position() const;
  double GetErrorRadius() const;

  bool IsModeChangeViewport() const;
  bool IsModeHasPosition() const;

  void SetRenderShape(drape_ptr<MyPosition> && shape);

  void SetFixedZoom();

  void NextMode();
  void TurnOf();
  void Invalidate();

  void OnLocationUpdate(location::GpsInfo const & info, bool isNavigable);
  void OnCompassUpdate(location::CompassInfo const & info);

  void SetModeListener(location::TMyPositionModeChanged const & fn);

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

private:
  void AnimateStateTransition(location::EMyPositionMode oldMode, location::EMyPositionMode newMode);
  void AnimateFollow();

  void Assign(location::GpsInfo const & info, bool isNavigable);
  bool Assign(location::CompassInfo const & info);
  void SetDirection(double bearing);

  void SetModeInfo(uint16_t modeInfo, bool force = false);
  location::EMyPositionMode GetMode() const;
  void CallModeListener(uint16_t mode);

  bool IsInRouting() const;
  bool IsRotationActive() const;

  void StopLocationFollow();
  void StopCompassFollow();
  void StopAllAnimations();

  bool IsVisible() const { return m_isVisible; }
  void SetIsVisible(bool isVisible) { m_isVisible = isVisible; }

private:
  // Mode bits
  // {
  static uint16_t const FixedZoomBit = 0x20;
  static uint16_t const RoutingSessionBit = 0x40;
  static uint16_t const KnownDirectionBit = 0x80;
  // }

  uint16_t m_modeInfo; // combination of Mode enum and "Mode bits"
  location::EMyPositionMode m_afterPendingMode;

  location::TMyPositionModeChanged m_modeChangeCallback;
  drape_ptr<MyPosition> m_shape;

  double m_errorRadius;   //< error radius in mercator
  m2::PointD m_position;  //< position in mercator
  double m_drawDirection;
  my::Timer m_lastGPSBearing;

  bool m_isVisible;
};

}
