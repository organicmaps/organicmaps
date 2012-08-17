#include "location_state.hpp"
#include "drawer_yg.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "rotate_screen_task.hpp"

#include "../anim/controller.hpp"

#include "../platform/location.hpp"
#include "../platform/platform.hpp"

#include "../geometry/rect2d.hpp"

#include "../indexer/mercator.hpp"

namespace location
{
  State::State(Framework * fw)
    : m_hasPosition(false),
      m_hasCompass(false),
      m_locationProcessMode(ELocationDoNothing),
      m_compassProcessMode(ECompassDoNothing),
      m_fw(fw)
  {
  }

  bool State::HasPosition() const
  {
    return m_hasPosition;
  }

  bool State::HasCompass() const
  {
    return m_hasCompass;
  }

  void State::TurnOff()
  {
    m_hasPosition = false;
    m_hasCompass = false;
  }

  void State::SkipLocationCentering()
  {
    m_locationProcessMode = ELocationSkipCentering;
  }

  ELocationProcessMode State::LocationProcessMode() const
  {
    return m_locationProcessMode;
  }

  void State::SetLocationProcessMode(ELocationProcessMode mode)
  {
    m_locationProcessMode = mode;
  }

  ECompassProcessMode State::CompassProcessMode() const
  {
    return m_compassProcessMode;
  }

  void State::SetCompassProcessMode(ECompassProcessMode mode)
  {
    m_compassProcessMode = mode;
  }

  void State::OnLocationStatusChanged(location::TLocationStatus newStatus)
  {
    switch (newStatus)
    {
    case location::EStarted:

      if (m_locationProcessMode != ELocationSkipCentering)
        m_locationProcessMode = ELocationCenterAndScale;
      break;

    case location::EFirstEvent:

      if (m_locationProcessMode != ELocationSkipCentering)
      {
        // set centering mode for the first location
        m_locationProcessMode = ELocationCenterAndScale;
        m_compassProcessMode = ECompassFollow;
      }
      break;

    default:
      m_locationProcessMode = ELocationDoNothing;
      TurnOff();
    }

    m_fw->Invalidate();
  }

  void State::OnGpsUpdate(location::GpsInfo const & info)
  {
    double lon = info.m_longitude;
    double lat = info.m_latitude;

    m2::RectD rect = MercatorBounds::MetresToXY(lon, lat, info.m_horizontalAccuracy);
    m2::PointD const center = rect.Center();

    m_hasPosition = true;
    m_position = center;
    m_errorRadius = rect.SizeX() / 2;

    switch (m_locationProcessMode)
    {
    case ELocationCenterAndScale:
    {
      int const rectScale = scales::GetScaleLevel(rect);
      int setScale = -1;

      // correct rect scale if country isn't downloaded
      int const upperScale = scales::GetUpperWorldScale();
      if (rectScale > upperScale && !m_fw->IsCountryLoaded(center))
        setScale = upperScale;
      else
      {
        // correct rect scale for best user experience
        int const bestScale = scales::GetUpperScale() - 1;
        if (rectScale > bestScale)
          setScale = bestScale;
      }

      if (setScale != -1)
        rect = scales::GetRectForLevel(setScale, center, 1.0);

      double a = m_fw->GetNavigator().Screen().GetAngle();
      double dx = rect.SizeX();
      double dy = rect.SizeY();

      m_fw->ShowRectFixed(m2::AnyRectD(rect.Center(), a, m2::RectD(-dx/2, -dy/2, dx/2, dy/2)));

      m_locationProcessMode = ELocationCenterOnly;
      break;
    }

    case ELocationCenterOnly:
      m_fw->SetViewportCenter(center);
      break;

    case ELocationSkipCentering:
      m_locationProcessMode = ELocationDoNothing;
      break;

    case ELocationDoNothing:
      break;
    }

    m_fw->Invalidate();
  }

  void State::OnCompassUpdate(location::CompassInfo const & info)
  {
    m_hasCompass = true;

    m_headingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);

    // Avoid situations when offset between magnetic north and true north is too small
    static double const MIN_SECTOR_RAD = math::pi / 18.0;
    m_headingHalfErrorRad = (info.m_accuracy < MIN_SECTOR_RAD ? MIN_SECTOR_RAD : info.m_accuracy);

    if (m_compassProcessMode == ECompassFollow)
      FollowCompass();

    m_fw->Invalidate();
  }

  void State::Draw(DrawerYG & drawer)
  {
    if (m_hasPosition)
    {
      m2::PointD const pxPosition = m_fw->GetNavigator().GtoP(Position());
      double const pxErrorRadius = pxPosition.Length(m_fw->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));

      // my position symbol
      drawer.drawSymbol(pxPosition, "current-position", yg::EPosCenter, yg::maxDepth);

      // my position circle
      drawer.screen()->fillSector(pxPosition, 0, 2.0 * math::pi, pxErrorRadius,
                                  yg::Color(0, 0, 255, 32),
                                  yg::maxDepth - 3);

      // display compass only if position is available
      double orientationRadius = max(pxErrorRadius, 30.0 * drawer.VisualScale());

      double screenAngle = m_fw->GetNavigator().Screen().GetAngle();

      // 0 angle is for North ("up"), but in our coordinates it's to the right.
      double headingRad = m_headingRad - math::pi / 2.0;

      if (m_hasCompass)
      {
        drawer.screen()->drawSector(pxPosition,
            screenAngle + headingRad - m_headingHalfErrorRad,
            screenAngle + headingRad + m_headingHalfErrorRad,
            orientationRadius,
            yg::Color(255, 255, 255, 192),
            yg::maxDepth);

        drawer.screen()->fillSector(pxPosition,
            screenAngle + headingRad - m_headingHalfErrorRad,
            screenAngle + headingRad + m_headingHalfErrorRad,
            orientationRadius,
            yg::Color(255, 255, 255, 96),
            yg::maxDepth - 1);
      }
    }
  }

  void State::FollowCompass()
  {
    if (!m_fw->GetNavigator().DoSupportRotation())
      return;

    m_fw->GetRenderPolicy()->GetAnimController()->Lock();

    StopAnimation();

    double startAngle = m_fw->GetNavigator().Screen().GetAngle();
    double endAngle = -m_headingRad;

    double period = 2 * math::pi;

    startAngle -= floor(startAngle / period) * period;
    endAngle -= floor(endAngle / period) * period;

    if (fabs(startAngle - endAngle) > 20.0 / 180.0 * math::pi)
    {
      if (fabs(startAngle - endAngle) > math::pi)
        startAngle -= 2 * math::pi;

      m_rotateScreenTask.reset(new RotateScreenTask(m_fw,
                                                    startAngle,
                                                    endAngle,
                                                    1));

      m_fw->GetRenderPolicy()->GetAnimController()->AddTask(m_rotateScreenTask);
    }

    m_fw->GetRenderPolicy()->GetAnimController()->Unlock();
  }

  void State::StopAnimation()
  {
    if (m_rotateScreenTask && !m_rotateScreenTask->IsFinished())
      m_rotateScreenTask->Finish();
    m_rotateScreenTask.reset();
  }
}
