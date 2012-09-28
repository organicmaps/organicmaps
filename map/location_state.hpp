#pragma once

#include "compass_filter.hpp"

#include "../platform/location.hpp"

#include "../geometry/point2d.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"

#include "../gui/element.hpp"

class Framework;
class RotateScreenTask;

namespace anim
{
  class AngleInterpolation;
}

namespace yg
{
  namespace gl
  {
    class DisplayList;
  }
}

namespace location
{
  class GpsInfo;
  class CompassInfo;

  enum ELocationProcessMode
  {
    ELocationDoNothing,
    ELocationCenterAndScale,
    ELocationCenterOnly,
    ELocationSkipCentering
  };

  enum ECompassProcessMode
  {
    ECompassDoNothing,
    ECompassFollow
  };

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State : public gui::Element
  {
  private:

    double m_errorRadius; //< error radius in mercator
    m2::PointD m_position; //< position in mercator

    CompassFilter m_compassFilter;
    double m_drawHeading;

    bool m_hasPosition;
    bool m_hasCompass;

    bool m_isCentered;

    ELocationProcessMode m_locationProcessMode;
    ECompassProcessMode m_compassProcessMode;

    void FollowCompass();

    /// GUI element related fields.

    typedef gui::Element base_t;

    yg::Color m_locationAreaColor;
    yg::Color m_locationBorderColor;

    math::Matrix<double, 3, 3> m_locationDrawM;

    yg::Color m_compassAreaColor;
    yg::Color m_compassBorderColor;

    math::Matrix<double, 3, 3> m_compassDrawM;

    Framework * m_framework;

    double m_cacheRadius;

    /// Compass Rendering Parameters
    /// @{

    double m_arrowHeight;
    double m_arrowWidth;
    double m_arrowBackHeight;
    double m_arrowScale;

    map<EState, shared_ptr<yg::gl::DisplayList> > m_arrowBodyLists;
    map<EState, shared_ptr<yg::gl::DisplayList> > m_arrowBorderLists;

    /// @}

    void cacheArrowBorder(EState state);
    void cacheArrowBody(EState state);

    void cache();
    void purge();
    void update();

    mutable vector<m2::AnyRectD> m_boundRects;
    m2::RectD m_boundRect;

    shared_ptr<anim::AngleInterpolation> m_headingInterpolation;

  public:

    struct Params : base_t::Params
    {
      yg::Color m_locationAreaColor;
      yg::Color m_locationBorderColor;
      yg::Color m_compassAreaColor;
      yg::Color m_compassBorderColor;
      Framework * m_framework;
    };

    State(Params const & p);

    /// @return GPS center point in mercator
    m2::PointD const & Position() const { return m_position; }

    bool HasPosition() const;
    bool HasCompass() const;

    ELocationProcessMode LocationProcessMode() const;
    void SetLocationProcessMode(ELocationProcessMode mode);

    ECompassProcessMode CompassProcessMode() const;
    void SetCompassProcessMode(ECompassProcessMode mode);

    void TurnOff();

    void StopCompassFollowing();
    void SetIsCentered(bool flag);
    bool IsCentered() const;

    void CheckCompassRotation();
    void CheckFollowCompass();

    /// @name GPS location updates routine.
    //@{
    void SkipLocationCentering();
    void OnLocationStatusChanged(location::TLocationStatus newStatus);
    void OnGpsUpdate(location::GpsInfo const & info);
    void OnCompassUpdate(location::CompassInfo const & info);
    //@}

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    bool hitTest(m2::PointD const & pt) const;

    bool onTapEnded(m2::PointD const & p);
  };
}
