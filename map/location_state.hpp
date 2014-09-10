#pragma once

#include "../gui/element.hpp"

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/map.hpp"


class Framework;

namespace graphics { class DisplayList; }

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
    // Do not change the order
    enum Mode
    {
      UnknowPosition = 0x0,
      PendingPosition = 0x1,
      NotFollow = 0x2,
      Follow = 0x4,
      RotateAndFollow = 0x8,
    };

    typedef function<void(Mode)> TStateModeListener;
    typedef function<void (m2::PointD const &)> TPositionListener;

    State(Params const & p);

    /// @return GPS center point in mercator
    m2::PointD const & Position() const;

    Mode GetMode() const;
    bool IsModeChangeViewport() const;
    bool IsModeHasPosition() const;
    void SwitchToNextMode();
    void RestoreMode();

    int  AddStateModeListener(TStateModeListener const & l);
    void RemoveStateModeListener(int slotID);

    int  AddPositionChangedListener(TPositionListener const & func);
    void RemovePositionChangedListener(int slotID);

    void TurnOff();
    void StopCompassFollowing();
    void StopLocationFollow();

    /// @name User input notification block
    //@{
    void DragStarted();
    void Draged();
    void DragEnded();

    void ScaleCorrection(m2::PointD & pt);
    void ScaleCorrection(m2::PointD & pt1, m2::PointD & pt2);

    void Rotated();
    //@}

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
    void AnimateCurrentState();

    void CallPositionChangedListeners(m2::PointD const & pt);
    void CallStateModeListeners();

    void CachePositionArrow();
    void CacheLocationMark();

    void FollowCompass();
    void SetModeInfo(uint16_t modeInfo);

  private:
    // Mode bits
    // {
    constexpr static uint16_t const ModeNotProcessed = 0x40;
    constexpr static uint16_t const KnownDirectionBit = 0x80;
    // }
    constexpr static float const s_cacheRadius = 500.0f;

    uint16_t m_modeInfo; // combination of Mode enum and "Mode bits"
    uint16_t m_dragModeInfo;
    Framework * m_framework;

    double m_errorRadius;   //< error radius in mercator
    m2::PointD m_position;  //< position in mercator
    double m_drawDirection;

    typedef map<int, TStateModeListener> TModeListeners;
    typedef map<int, TPositionListener> TPositionListeners;

    TModeListeners m_modeListeners;
    TPositionListeners m_positionListeners;
    int m_currentSlotID;

    /// @nameCompass Rendering Parameters
    //@{
    unique_ptr<graphics::DisplayList> m_positionArrow;
    unique_ptr<graphics::DisplayList> m_locationMarkDL;
    unique_ptr<graphics::DisplayList> m_positionMarkDL;
    graphics::Color m_locationAreaColor;
    //@}
  };
}
