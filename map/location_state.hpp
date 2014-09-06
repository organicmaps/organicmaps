#pragma once

#include "../gui/element.hpp"

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/map.hpp"


class Framework;

namespace graphics { class DisplayList; }
namespace anim { class Task; }

namespace location
{
  class GpsInfo;
  class CompassInfo;

  enum ELocationProcessMode
  {
    ELocationDoNothing = 0,
    ELocationCenterAndScale,
    ELocationCenterOnly
  };

  enum ECompassProcessMode
  {
    ECompassDoNothing = 0,
    ECompassFollow
  };

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State : public gui::Element
  {
  public:
    typedef function<void(int)> TCompassStatusListener;
    typedef function<void (m2::PointD const &)> TPositionChangedCallback;

  private:

    static const double s_cacheRadius;

    double m_errorRadius;   //< error radius in mercator
    m2::PointD m_position;  //< position in mercator
    m2::PointD m_halfArrowSize; //< size of Arrow image

    double m_drawHeading;

    bool m_hasPosition;
    double m_positionFault;
    bool m_hasCompass;
    double m_compassFault;
    bool m_isCentered;
    bool m_isFirstPosition;

    bool IsPositionFaultCritical() const;
    bool IsCompassFaultCritical() const;

    typedef map<int, TCompassStatusListener> TCompassStatusListeners;
    TCompassStatusListeners m_compassStatusListeners;

    typedef map<int, TPositionChangedCallback> TPositionChangedListeners;
    TPositionChangedListeners m_callbacks;
    int m_currentSlotID;

    void CallPositionChangedListeners(m2::PointD const & pt);
    void CallCompassStatusListeners(ECompassProcessMode mode);

    ELocationProcessMode m_locationProcessMode;
    ECompassProcessMode m_compassProcessMode;

    typedef gui::Element BaseT;

    graphics::Color m_locationAreaColor;

    Framework * m_framework;

    /// @nameCompass Rendering Parameters
    //@{
    unique_ptr<graphics::DisplayList> m_positionArrow;
    unique_ptr<graphics::DisplayList> m_locationMarkDL;
    unique_ptr<graphics::DisplayList> m_positionMarkDL;
    //@}

    void cachePositionArrow();
    void cacheLocationMark();

    void cache();
    void purge();
    void update();

    m2::RectD m_boundRect;

    void CheckCompassFollowing();
    void FollowCompass();

  public:
    struct Params : BaseT::Params
    {
      graphics::Color m_locationAreaColor;
      Framework * m_framework;
      Params();
    };

    State(Params const & p);

    /// @return GPS center point in mercator
    m2::PointD const & Position() const;

    bool HasPosition() const;
    bool HasCompass() const;
    bool IsFirstPosition() const;

    ELocationProcessMode GetLocationProcessMode() const;
    void SetLocationProcessMode(ELocationProcessMode mode);

    ECompassProcessMode GetCompassProcessMode() const;
    void SetCompassProcessMode(ECompassProcessMode mode);

    int  AddCompassStatusListener(TCompassStatusListener const & l);
    void RemoveCompassStatusListener(int slotID);

    int  AddPositionChangedListener(TPositionChangedCallback const & func);
    void RemovePositionChangedListener(int slotID);

    void TurnOff();

    void StartCompassFollowing();
    void StopCompassFollowing();

    void OnStartLocation();
    void OnStopLocation();

    void SetIsCentered(bool flag);
    bool IsCentered() const;

    void AnimateToPosition();
    void AnimateToPositionAndEnqueueFollowing();
    void AnimateToPositionAndEnqueueLocationProcessMode(location::ELocationProcessMode mode);

    /// @name GPS location updates routine.
    //@{
    void OnLocationUpdate(location::GpsInfo const & info);
    void OnCompassUpdate(location::CompassInfo const & info);
    //@}

    /// @name Override from graphics::OverlayElement and gui::Element.
    //@{
    virtual m2::RectD GetBoundRect() const;

    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    bool roughHitTest(m2::PointD const & pt) const;
    bool hitTest(m2::PointD const & pt) const;
    //@}
  };
}
