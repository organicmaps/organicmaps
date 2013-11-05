#pragma once

#include "../gui/element.hpp"

#include "../platform/location.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/map.hpp"


class Framework;

namespace graphics { class DisplayList; }

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
    typedef function<void(m2::PointD const &)> TOnPositionClickListener;

  private:

    static const double s_cacheRadius;

    double m_errorRadius;   //< error radius in mercator
    m2::PointD m_position;  //< position in mercator

    double m_drawHeading;

    bool m_hasPosition;
    bool m_hasCompass;
    bool m_isCentered;
    bool m_isFirstPosition;

    ELocationProcessMode m_locationProcessMode;
    ECompassProcessMode m_compassProcessMode;

    typedef gui::Element base_t;

    graphics::Color m_locationAreaColor;

    graphics::Color m_compassAreaColor;
    graphics::Color m_compassBorderColor;

    Framework * m_framework;

    /// Compass Rendering Parameters
    /// @{

    double m_arrowHeight;
    double m_arrowWidth;
    double m_arrowBackHeight;
    double m_arrowScale;

    map<EState, shared_ptr<graphics::DisplayList> > m_arrowBodyLists;
    map<EState, shared_ptr<graphics::DisplayList> > m_arrowBorderLists;
    scoped_ptr<graphics::DisplayList> m_locationMarkDL;
    scoped_ptr<graphics::DisplayList> m_positionMarkDL;

    /// @}

    void cacheArrowBorder(EState state);
    void cacheArrowBody(EState state);
    void cacheLocationMark();

    void cache();
    void purge();
    void update();

    mutable vector<m2::AnyRectD> m_boundRects;
    m2::RectD m_boundRect;

    typedef map<int, TCompassStatusListener> TCompassStatusListeners;
    TCompassStatusListeners m_compassStatusListeners;
    int m_currentSlotID;

    typedef map<int, TOnPositionClickListener> TOnPositionClickListeners;
    TOnPositionClickListeners m_onPositionClickListeners;

    void CallOnPositionClickListeners(m2::PointD const & point);

    void CallCompassStatusListeners(ECompassProcessMode mode);

    double ComputeMoveSpeed(m2::PointD const & globalPt0,
                            m2::PointD const & globalPt1,
                            ScreenBase const & s);

    void CheckCompassFollowing();
    void FollowCompass();

  public:
    struct Params : base_t::Params
    {
      graphics::Color m_locationAreaColor;
      graphics::Color m_compassAreaColor;
      graphics::Color m_compassBorderColor;
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

    void TurnOff();

    void StartCompassFollowing();
    void StopCompassFollowing();

    void OnStartLocation();
    void OnStopLocation();

    int AddCompassStatusListener(TCompassStatusListener const & l);
    void RemoveCompassStatusListener(int slotID);

    int AddOnPositionClickListener(TOnPositionClickListener const & listner);
    void RemoveOnPositionClickListener(int listnerID);

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

    /// graphics::OverlayElement and gui::Element related methods
    // @{
    vector<m2::AnyRectD> const & boundRects() const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    bool roughHitTest(m2::PointD const & pt) const;
    bool hitTest(m2::PointD const & pt) const;
    bool onTapEnded(m2::PointD const & p);
    /// @}
  };
}
