#pragma once

#include "geometry/point2d.hpp"

#include "base/timer.hpp"

#include "routing/turns.hpp"

#include "platform/location.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/map.hpp"

class Framework;
class ScreenBase;

namespace anim { class Task;}

namespace location
{
  class GpsInfo;
  class CompassInfo;

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State
  {

  public:
    struct Params
    {
      ///@TODO UVR
      //graphics::Color m_locationAreaColor;
      Framework * m_framework;
      Params();
    };

    // Do not change the order and values
    enum Mode
    {
      UnknownPosition = 0x0,
      PendingPosition = 0x1,
      NotFollow = 0x2,
      Follow = 0x3,
      RotateAndFollow = 0x4,
    };

    typedef function<void(Mode)> TStateModeListener;
    typedef function<void (m2::PointD const &)> TPositionListener;

    State(Params const & p);

    /// @return GPS center point in mercator
    m2::PointD const & Position() const;
    double GetErrorRadius() const;
    double GetDirection() const { return m_drawDirection; }
    bool IsDirectionKnown() const;

    Mode GetMode() const;
    bool IsModeChangeViewport() const;
    bool IsModeHasPosition() const;
    void SwitchToNextMode();

    void RouteBuilded();
    void StartRouteFollow(int scale);
    void StopRoutingMode();

    int  AddStateModeListener(TStateModeListener const & l);
    void RemoveStateModeListener(int slotID);

    int  AddPositionChangedListener(TPositionListener const & func);
    void RemovePositionChangedListener(int slotID);

    void InvalidatePosition();
    void TurnOff();
    void StopCompassFollowing();
    void StopLocationFollow(bool callListeners = true);
    void SetFixedZoom();
    void SetRoutingNotFollow();

    /// @name User input notification block
    //@{
    void DragStarted();
    void DragEnded();

    void ScaleStarted();
    void CorrectScalePoint(m2::PointD & pt) const;
    void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const;
    void ScaleEnded();

    void Rotated();
    //@}

    void OnCompassTaped();

    void OnSize();

    /// @name GPS location updates routine.
    //@{
    void OnLocationUpdate(location::GpsInfo const & info, bool isNavigable, location::RouteMatchingInfo const & routeMatchingInfo);
    void OnCompassUpdate(location::CompassInfo const & info);
    //@}

    RouteMatchingInfo const & GetRouteMatchingInfo() const { return m_routeMatchingInfo; }
    void ResetRouteMatchingInfo() { m_routeMatchingInfo.Reset(); }
    void ResetDirection();

    /// @name Override from graphics::OverlayElement and gui::Element.
    //@{
    ///@TODO UVR
    //virtual m2::RectD GetBoundRect() const { return m2::RectD(); }

    bool hitTest(m2::PointD const & /*pt*/) const { return false; }

    void cache();
    void purge();
    void update();
    //@}

  private:
    void AnimateStateTransition(Mode oldMode, Mode newMode);
    void AnimateFollow();

    void RotateOnNorth();

    void CallPositionChangedListeners(m2::PointD const & pt);
    void CallStateModeListeners();

    bool IsRotationActive() const;
    bool IsInRouting() const;
    bool IsRoutingFollowingDisabled() const;

    m2::PointD const GetModeDefaultPixelBinding(Mode mode) const;
    m2::PointD const GetRaFModeDefaultPxBind() const;

    void SetModeInfo(uint16_t modeInfo, bool callListeners = true);

    void StopAllAnimations();

    ScreenBase const & GetModelView() const;

    void Assign(location::GpsInfo const & info, bool isNavigable);
    bool Assign(location::CompassInfo const & info);
    void SetDirection(double bearing);
    const m2::PointD GetPositionForDraw() const;

  private:
    
    enum ExternalMode
    {
      RoutingNotFollowBit = 0x10,
      FixedZoomBit = 0x20,
      RoutingSessionBit = 0x40,
      KnownDirectionBit = 0x80
    };
    static uint16_t const s_cacheRadius = 500.0f;

    uint16_t m_modeInfo; // combination of Mode enum and "Mode bits"
    uint16_t m_dragModeInfo = 0;
    uint16_t m_scaleModeInfo = 0;
    Framework * m_framework;

    double m_errorRadius;   //< error radius in mercator
    m2::PointD m_position;  //< position in mercator
    double m_drawDirection;
    my::Timer m_lastGPSBearing;
    Mode m_afterPendingMode;

    RouteMatchingInfo m_routeMatchingInfo;

    typedef map<int, TStateModeListener> TModeListeners;
    typedef map<int, TPositionListener> TPositionListeners;

    TModeListeners m_modeListeners;
    TPositionListeners m_positionListeners;
    int m_currentSlotID;

    /// @name Compass Rendering Parameters
    //@{
    //@}

    /// @name Rotation mode animation
    //@{
    shared_ptr<anim::Task> m_animTask;
    bool FollowCompass();
    void CreateAnimTask();
    void CreateAnimTask(m2::PointD const & srcPx, m2::PointD const & dstPx);
    void EndAnimation();
    //@}
  };
}
