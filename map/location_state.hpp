#pragma once

#include "../gui/element.hpp"

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/map.hpp"


class Framework;

namespace graphics { class DisplayList; }
namespace anim { class Task;}

namespace location
{
  class GpsInfo;
  class CompassInfo;

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State : public gui::Element
  {
    typedef gui::Element TBase;

  public:
    struct Params : TBase::Params
    {
      graphics::Color m_locationAreaColor;
      Framework * m_framework;
      Params();
    };

  public:
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

    Mode GetMode() const;
    bool IsModeChangeViewport() const;
    bool IsModeHasPosition() const;
    void SwitchToNextMode();

    void StartRoutingMode();
    void StopRoutingMode();

    int  AddStateModeListener(TStateModeListener const & l);
    void RemoveStateModeListener(int slotID);

    int  AddPositionChangedListener(TPositionListener const & func);
    void RemovePositionChangedListener(int slotID);

    void InvalidatePosition();
    void TurnOff();
    void StopCompassFollowing();
    void StopLocationFollow();

    /// @name User input notification block
    //@{
    void DragStarted();
    void Draged();
    void DragEnded();

    void ScaleStarted();
    void ScaleCorrection(m2::PointD & pt);
    void ScaleCorrection(m2::PointD & pt1, m2::PointD & pt2);
    void ScaleEnded();

    void Rotated();
    //@}

    void OnSize(m2::RectD const & oldPixelRect);

    /// @name GPS location updates routine.
    //@{
    void OnLocationUpdate(location::GpsInfo const & info);
    void OnCompassUpdate(location::CompassInfo const & info);
    //@}

    /// @name Override from graphics::OverlayElement and gui::Element.
    //@{
    virtual m2::RectD GetBoundRect() const { return m2::RectD(); }

    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    bool hitTest(m2::PointD const & /*pt*/) const { return false; }

    void cache();
    void purge();
    void update();
    //@}

  private:
    void AnimateStateTransition(Mode oldMode, Mode newMode);
    void AnimateFollow();

    void CallPositionChangedListeners(m2::PointD const & pt);
    void CallStateModeListeners();

    void CachePositionArrow();
    void CacheRoutingArrow();
    void CacheLocationMark();

    void CacheArrow(graphics::DisplayList * dl, string const & iconName);

    bool IsRotationActive() const;
    bool IsDirectionKnown() const;
    bool IsInRouting() const;

    m2::PointD const GetModeDefaultPixelBinding(Mode mode) const;
    m2::PointD const GetRaFModeDefaultPxBind() const;

    void SetModeInfo(uint16_t modeInfo);

    void StopAllAnimations();

  private:
    // Mode bits
    // {
    static uint16_t const RoutingSessionBit = 0x40;
    static uint16_t const KnownDirectionBit = 0x80;
    // }
    static uint16_t const s_cacheRadius = 500.0f;

    uint16_t m_modeInfo; // combination of Mode enum and "Mode bits"
    uint16_t m_dragModeInfo = 0;
    uint16_t m_scaleModeInfo = 0;
    Framework * m_framework;

    double m_errorRadius;   //< error radius in mercator
    m2::PointD m_position;  //< position in mercator
    double m_drawDirection;
    Mode m_afterPendingMode;

    typedef map<int, TStateModeListener> TModeListeners;
    typedef map<int, TPositionListener> TPositionListeners;

    TModeListeners m_modeListeners;
    TPositionListeners m_positionListeners;
    int m_currentSlotID;

    /// @name Compass Rendering Parameters
    //@{
    unique_ptr<graphics::DisplayList> m_positionArrow;
    unique_ptr<graphics::DisplayList> m_locationMarkDL;
    unique_ptr<graphics::DisplayList> m_positionMarkDL;
    unique_ptr<graphics::DisplayList> m_routingArrow;
    graphics::Color m_locationAreaColor;
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
