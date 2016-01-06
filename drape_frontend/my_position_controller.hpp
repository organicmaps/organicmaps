#pragma once

#include "drape_frontend/my_position.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"

#include "base/timer.hpp"

#include "std/function.hpp"

namespace df
{
class BaseModelViewAnimation;

class MyPositionController
{
public:
  class Listener
  {
  public:
    virtual ~Listener() {}
    virtual void PositionChanged(m2::PointD const & position) = 0;
    /// Show map with center in "center" point and current zoom
    virtual void ChangeModelView(m2::PointD const & center) = 0;
    /// Change azimuth of current ModelView
    virtual void ChangeModelView(double azimuth) = 0;
    /// Somehow show map that "rect" will see
    virtual void ChangeModelView(m2::RectD const & rect) = 0;
    /// Show map where "usePos" (mercator) placed in "pxZero" on screen and map rotated around "userPos"
    virtual void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero,
                                 int preferredZoomLevel) = 0;
  };

  // Render bits
  enum RenderMode
  {
    RenderAccuracy = 0x1,
    RenderMyPosition = 0x2
  };

  MyPositionController(location::EMyPositionMode initMode);
  ~MyPositionController();

  void OnNewPixelRect();
  void UpdatePixelPosition(ScreenBase const & screen);
  void SetListener(ref_ptr<Listener> listener);

  m2::PointD const & Position() const;
  double GetErrorRadius() const;

  bool IsModeChangeViewport() const;
  bool IsModeHasPosition() const;

  void DragStarted();
  void DragEnded(m2::PointD const & distance);

  void AnimationStarted(ref_ptr<BaseModelViewAnimation> anim);
  void ScaleStarted();
  void Rotated();
  void CorrectScalePoint(m2::PointD & pt) const;
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const;
  void CorrectGlobalScalePoint(m2::PointD & pt) const;
  void ScaleEnded();

  void SetRenderShape(drape_ptr<MyPosition> && shape);

  void SetFixedZoom();

  void ActivateRouting();
  void DeactivateRouting();

  void StopLocationFollow();
  bool StopCompassFollow();
  void NextMode(int preferredZoomLevel = -1);
  void TurnOff();
  void Invalidate();

  void OnLocationUpdate(location::GpsInfo const & info, bool isNavigable, ScreenBase const & screen);
  void OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen);

  void SetModeListener(location::TMyPositionModeChanged const & fn);

  void Render(uint32_t renderMode, ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  bool IsFollowingActive() const;
  bool IsRotationActive() const;
  bool IsInRouting() const;

private:
  void AnimateStateTransition(location::EMyPositionMode oldMode, location::EMyPositionMode newMode);

  void Assign(location::GpsInfo const & info, bool isNavigable, ScreenBase const & screen);
  void Assign(location::CompassInfo const & info, ScreenBase const & screen);
  void SetDirection(double bearing);

  void SetModeInfo(uint32_t modeInfo, bool force = false);
  location::EMyPositionMode GetMode() const;
  void CallModeListener(uint32_t mode);

  bool IsVisible() const { return m_isVisible; }
  void SetIsVisible(bool isVisible) { m_isVisible = isVisible; }

  void ChangeModelView(m2::PointD const & center);
  void ChangeModelView(double azimuth);
  void ChangeModelView(m2::RectD const & rect);
  void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero,
                       int preferredZoomLevel);

  void Follow(int preferredZoomLevel = -1);
  m2::PointD GetRaFPixelBinding() const;
  m2::PointD GetCurrentPixelBinding() const;

  m2::PointD GetDrawablePosition() const;
  double GetDrawableAzimut() const;
  void CheckAnimFinished() const;
  void CreateAnim(m2::PointD const & oldPos, double oldAzimut, ScreenBase const & screen);

  bool AlmostCurrentPosition(m2::PointD const & pos) const;
  bool AlmostCurrentAzimut(double azimut) const;

private:
  // Mode bits
  // {
  static uint32_t const FixedZoomBit = 0x20;
  static uint32_t const RoutingSessionBit = 0x40;
  static uint32_t const KnownDirectionBit = 0x80;
  static uint32_t const BlockAnimation = 0x100;
  static uint32_t const StopFollowOnActionEnd = 0x200;
  // }

  uint32_t m_modeInfo; // combination of Mode enum and "Mode bits"
  location::EMyPositionMode m_afterPendingMode;

  location::TMyPositionModeChanged m_modeChangeCallback;
  drape_ptr<MyPosition> m_shape;
  ref_ptr<Listener> m_listener;

  double m_errorRadius;  // error radius in mercator
  m2::PointD m_position; // position in mercator
  double m_drawDirection;
  my::HighResTimer m_lastGPSBearing;

  m2::RectD m_pixelRect;
  m2::PointD m_pixelPositionRaF;
  m2::PointD m_pixelPositionF;
  double m_positionYOffset;

  bool m_isVisible;
  bool m_isDirtyViewport;
  bool m_isPendingAnimation = false;

  m2::PointD m_oldPosition; // position in mercator
  double m_oldDrawDirection;

  class MyPositionAnim;
  mutable drape_ptr<MyPositionAnim> m_anim;

  using TAnimationCreator = function<void()>;
  TAnimationCreator m_animCreator;
};

}
