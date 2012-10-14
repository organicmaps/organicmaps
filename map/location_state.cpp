#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "compass_filter.hpp"
#include "change_viewport_task.hpp"

#include "../yg/display_list.hpp"
#include "../yg/skin.hpp"

#include "../anim/controller.hpp"
#include "../anim/angle_interpolation.hpp"

#include "../gui/controller.hpp"

#include "../platform/location.hpp"
#include "../platform/platform.hpp"
#include "../platform/settings.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/transformations.hpp"

#include "../indexer/mercator.hpp"

#include "../base/logging.hpp"

namespace location
{
  State::Params::Params()
    : m_useDirectionArrow(true)
  {}

  State::State(Params const & p)
    : base_t(p),
      m_errorRadius(0),
      m_position(0, 0),
      m_hasPosition(false),
      m_hasCompass(false),
      m_isCentered(false),
      m_locationProcessMode(ELocationDoNothing),
      m_compassProcessMode(ECompassDoNothing),
      m_currentSlotID(0)
  {
    m_drawHeading = m_compassFilter.GetHeadingRad();
    m_locationAreaColor = p.m_locationAreaColor;
    m_locationBorderColor = p.m_locationBorderColor;
    m_compassAreaColor = p.m_compassAreaColor;
    m_compassBorderColor = p.m_compassBorderColor;
    m_useDirectionArrow = p.m_useDirectionArrow;
    m_framework = p.m_framework;

    /// @todo Probably we can make this like static const int.
    /// It's not a class state, so no need to store it in memory.
    m_arrowScale = 0.7;
    m_arrowWidth = 40 * m_arrowScale;
    m_arrowHeight = 50 * m_arrowScale;
    m_arrowBackHeight = 10 * m_arrowScale;

    m_boundRects.resize(1);

    setColor(EActive, yg::Color(0x2f, 0xb5, 0xea, 128));
    setColor(EPressed, yg::Color(0x1f, 0x22, 0x59, 128));
    setState(EActive);
    setIsVisible(false);
  }

  bool State::HasPosition() const
  {
    return m_hasPosition;
  }

  m2::PointD const & State::Position() const
  {
    return m_position;
  }

  bool State::HasCompass() const
  {
    return m_hasCompass;
  }

  bool State::IsFirstPosition() const
  {
    return m_isFirstPosition;
  }

  void State::TurnOff()
  {
    m_hasPosition = false;
    m_hasCompass = false;
    setIsVisible(false);
    invalidate();
  }

  ELocationProcessMode State::GetLocationProcessMode() const
  {
    return m_locationProcessMode;
  }

  void State::SetLocationProcessMode(ELocationProcessMode mode)
  {
    m_locationProcessMode = mode;
  }

  ECompassProcessMode State::GetCompassProcessMode() const
  {
    return m_compassProcessMode;
  }

  void State::SetCompassProcessMode(ECompassProcessMode mode)
  {
    bool stateChanged = (m_compassProcessMode != mode);

    m_compassProcessMode = mode;

    if (stateChanged)
      CallCompassStatusListeners(mode);
  }

  void State::OnLocationUpdate(location::GpsInfo const & info)
  {
    m_isFirstPosition = false;

    m2::RectD rect = MercatorBounds::MetresToXY(info.m_longitude,
                                                info.m_latitude,
                                                info.m_horizontalAccuracy);
    m2::PointD const center = rect.Center();

    m_hasPosition = true;
    setIsVisible(true);
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
      if (rectScale > upperScale && !m_framework->IsCountryLoaded(center))
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

      m_framework->ShowRectEx(rect);

      SetIsCentered(true);
      CheckCompassRotation();
      CheckCompassFollowing();

      m_locationProcessMode = ELocationCenterOnly;
      break;
    }

    case ELocationCenterOnly:

      m_framework->SetViewportCenter(center);

      SetIsCentered(true);
      CheckCompassRotation();
      CheckCompassFollowing();

      break;

    case ELocationDoNothing:
      break;
    }

    invalidate();
  }

  void State::OnCompassUpdate(location::CompassInfo const & info)
  {
    m_hasCompass = true;

    m_compassFilter.OnCompassUpdate(info);

    CheckCompassRotation();
    CheckCompassFollowing();

    invalidate();
  }

  vector<m2::AnyRectD> const & State::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects[0] = m2::AnyRectD(m_boundRect);
      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void State::cacheArrowBorder(EState state)
  {
    yg::gl::Screen * cacheScreen = m_controller->GetCacheScreen();

    shared_ptr<yg::gl::DisplayList> & dl = m_arrowBorderLists[state];

    dl.reset();
    dl.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(dl.get());

    shared_ptr<yg::Skin> const & skin = cacheScreen->skin();

    double k = m_controller->GetVisualScale();

    m2::PointD ptsD[5] =
    {
      m2::PointD(0, 0),
      m2::PointD(-(m_arrowWidth * k) / 2, (m_arrowBackHeight * k)),
      m2::PointD(0, -m_arrowHeight * k + m_arrowBackHeight * k),
      m2::PointD((m_arrowWidth * k) / 2, m_arrowBackHeight * k),
      m2::PointD(0, 0)
    };

    yg::Color borderColor = m_locationAreaColor;
    borderColor.a = 255;

    uint32_t penStyle = skin->mapPenInfo(yg::PenInfo(borderColor, 1 * k, 0, 0, 0));

    cacheScreen->drawPath(ptsD, 5, 0, penStyle, depth());

    cacheScreen->setDisplayList(0);
    cacheScreen->endFrame();
  }

  void State::cacheArrowBody(EState state)
  {
    yg::gl::Screen * cacheScreen = m_controller->GetCacheScreen();

    shared_ptr<yg::gl::DisplayList> & dl = m_arrowBodyLists[state];

    dl.reset();
    dl.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(dl.get());

    double k = m_controller->GetVisualScale();

    m2::PointF pts[4] =
    {
      m2::PointF(0, 0),
      m2::PointF(-(m_arrowWidth * k) / 2, (m_arrowBackHeight * k)),
      m2::PointF(0, -m_arrowHeight * k + m_arrowBackHeight * k),
      m2::PointF((m_arrowWidth * k) / 2, m_arrowBackHeight * k),
    };

    shared_ptr<yg::Skin> const & skin = cacheScreen->skin();

    uint32_t colorStyle = skin->mapColor(color(state));

    cacheScreen->drawTrianglesFan(pts, 4,
                                  colorStyle,
                                  depth());


    cacheScreen->setDisplayList(0);
    cacheScreen->endFrame();
  }

  void State::cache()
  {
    cacheArrowBody(EActive);
    cacheArrowBorder(EActive);
    cacheArrowBody(EPressed);
    cacheArrowBorder(EPressed);

    m_controller->GetCacheScreen()->completeCommands();
  }

  void State::purge()
  {
    m_arrowBorderLists.clear();
    m_arrowBodyLists.clear();
  }

  void State::update()
  {
    if (isVisible() && m_hasPosition)
    {
      m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());

      setPivot(pxPosition);

      double const pxErrorRadius = pxPosition.Length(m_framework->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));

      m2::RectD newRect(pxPosition - m2::PointD(pxErrorRadius, pxErrorRadius),
                        pxPosition + m2::PointD(pxErrorRadius, pxErrorRadius));

      double const pxArrowRadius = m_arrowHeight * m_controller->GetVisualScale();

      m2::RectD arrowRect(pxPosition - m2::PointD(pxArrowRadius, pxArrowRadius),
                          pxPosition + m2::PointD(pxArrowRadius, pxArrowRadius));

      newRect.Add(arrowRect);

      if (newRect != m_boundRect)
      {
        m_boundRect = newRect;
        setIsDirtyRect(true);
      }
    }
  }

  void State::draw(yg::gl::OverlayRenderer * r,
                   math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyDrawing();

      if (m_hasPosition)
      {
        double screenAngle = m_framework->GetNavigator().Screen().GetAngle();
        math::Matrix<double, 3, 3> compassDrawM;

        /// drawing arrow body first
        if (m_hasCompass && m_useDirectionArrow)
        {
          double const headingRad = m_drawHeading;

          double k = m_controller->GetVisualScale();

          compassDrawM =
              math::Shift(
                math::Rotate(
                  math::Shift(
                    math::Identity<double, 3>(),
                    m2::PointD(0, 0.4 * m_arrowHeight * k - m_arrowBackHeight * k)),
                  screenAngle + headingRad),
                pivot());

          map<EState, shared_ptr<yg::gl::DisplayList> >::const_iterator it;
          it = m_arrowBodyLists.find(state());

          if (it != m_arrowBodyLists.end())
            it->second->draw(compassDrawM * m);
          else
            LOG(LWARNING, ("m_compassDisplayLists[state()] is not set!"));
        }

        /// then position
        m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
        double const pxErrorRadius = pxPosition.Length(
              m_framework->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));


        if (!m_hasCompass || !m_useDirectionArrow)
          r->drawSymbol(pxPosition,
                       "current-position",
                        yg::EPosCenter,
                        depth() - 2);

        r->fillSector(pxPosition,
                      0, 2.0 * math::pi,
                      pxErrorRadius,
                      m_locationAreaColor,
                      depth() - 3);

        /// and then arrow border
        if (m_hasCompass && m_useDirectionArrow)
        {
          map<EState, shared_ptr<yg::gl::DisplayList> >::const_iterator it;
          it = m_arrowBorderLists.find(state());

          if (it != m_arrowBorderLists.end())
            it->second->draw(compassDrawM * m);
          else
            LOG(LWARNING, ("m_arrowBorderLists[state()] is not set!"));
        }

        /// @todo remove it
        if (m_hasCompass && !m_useDirectionArrow)
        {
          double drawHeadingError = m_compassFilter.GetHeadingHalfErrorRad();

          double a = screenAngle + m_drawHeading - math::pi / 2;
          double errorRadius = max(pxErrorRadius, 30 * m_controller->GetVisualScale());

          r->fillSector(pxPosition,
                        a - drawHeadingError,
                        a + drawHeadingError,
                        errorRadius,
                        m_compassAreaColor,
                        depth());

          r->drawSector(pxPosition,
                        a - drawHeadingError,
                        a + drawHeadingError,
                        errorRadius,
                        m_compassBorderColor,
                        depth());
        }
      }
    }
  }

  bool State::hitTest(m2::PointD const & pt) const
  {
    double radius = m_arrowHeight * m_controller->GetVisualScale();
    return m_hasCompass && (pt.SquareLength(pivot()) <= my::sq(radius));
  }

  void State::CheckCompassRotation()
  {
#ifndef OMIM_OS_IPHONE

    if (m_headingInterpolation)
      m_headingInterpolation->Lock();

    double headingDelta = 0;
    bool isRunning = m_headingInterpolation
                  && m_headingInterpolation->IsRunning();

    if (isRunning)
      headingDelta = fabs(ang::GetShortestDistance(m_headingInterpolation->EndAngle(), m_compassFilter.GetHeadingRad()));

    if (floor(ang::RadToDegree(headingDelta)) > 0)
      m_headingInterpolation->SetEndAngle(m_compassFilter.GetHeadingRad());
    else
    {
      if (!isRunning)
      {
        headingDelta = fabs(ang::GetShortestDistance(m_drawHeading, m_compassFilter.GetHeadingRad()));

        if (my::rounds(ang::RadToDegree(headingDelta)) > 0)
        {
          if (m_headingInterpolation
          && !m_headingInterpolation->IsCancelled()
          && !m_headingInterpolation->IsEnded())
          {
            m_headingInterpolation->Cancel();
            m_headingInterpolation->Unlock();
            m_headingInterpolation.reset();
          }

          m_headingInterpolation.reset(new anim::AngleInterpolation(m_drawHeading,
                                                                    m_compassFilter.GetHeadingRad(),
                                                                    m_framework->GetAnimator().GetRotationSpeed(),
                                                                    m_drawHeading));

          m_framework->GetAnimController()->AddTask(m_headingInterpolation);
          return;
        }
      }
    }

    if (m_headingInterpolation)
      m_headingInterpolation->Unlock();

#else
    m_drawHeading = m_compassFilter.GetHeadingRad();
#endif
  }

  void State::CheckCompassFollowing()
  {
    if (m_hasCompass
    && (GetCompassProcessMode() == ECompassFollow)
    && IsCentered())
      FollowCompass();
  }

  void State::FollowCompass()
  {
    if (!m_framework->GetNavigator().DoSupportRotation())
      return;

    anim::Controller * controller = m_framework->GetAnimController();

    controller->Lock();

    double startAngle = m_framework->GetNavigator().Screen().GetAngle();
    double endAngle = -m_compassFilter.GetHeadingRad();

    m_framework->GetAnimator().RotateScreen(startAngle, endAngle);

    controller->Unlock();
  }

  void State::AnimateToPosition()
  {
    anim::Controller * controller = m_framework->GetAnimController();

    controller->Lock();

    m2::AnyRectD startRect = m_framework->GetNavigator().Screen().GlobalRect();
    m2::AnyRectD endRect = m2::AnyRectD(Position(),
                                        startRect.Angle().val(),
                                        m2::RectD(startRect.GetLocalRect()));

    m_framework->GetAnimator().ChangeViewport(startRect, endRect, 2);

    controller->Unlock();
  }

  void State::AnimateToPositionAndEnqueueFollowing()
  {
    anim::Controller * controller = m_framework->GetAnimController();

    controller->Lock();

    m2::AnyRectD startRect = m_framework->GetNavigator().Screen().GlobalRect();
    m2::AnyRectD endRect = m2::AnyRectD(Position(),
                                        -m_compassFilter.GetHeadingRad(),
                                        m2::RectD(startRect.GetLocalRect()));

    shared_ptr<ChangeViewportTask> const & t = m_framework->GetAnimator().ChangeViewport(startRect, endRect, 2);

    t->Lock();
    t->AddCallback(anim::Task::EEnded, bind(&State::SetIsCentered, this, true));
    t->AddCallback(anim::Task::EEnded, bind(&State::StartCompassFollowing, this));
    t->Unlock();

    controller->Unlock();
  }

  void State::StartCompassFollowing()
  {
    SetCompassProcessMode(ECompassFollow);
    SetLocationProcessMode(ELocationCenterOnly);
    CheckCompassRotation();
    CheckCompassFollowing();
    setState(EPressed);
  }

  void State::StopCompassFollowing()
  {
    SetCompassProcessMode(ECompassDoNothing);
    m_framework->GetAnimator().StopRotation();
    m_framework->GetAnimator().StopChangeViewport();
    setState(EActive);
  }

  bool State::IsCentered() const
  {
    return m_isCentered;
  }

  void State::SetIsCentered(bool flag)
  {
    m_isCentered = flag;
  }

  void State::CallCompassStatusListeners(ECompassProcessMode mode)
  {
    for (TCompassStatusListeners::const_iterator it = m_compassStatusListeners.begin();
         it != m_compassStatusListeners.end();
         ++it)
      it->second(mode);
  }

  int State::AddCompassStatusListener(TCompassStatusListener const & l)
  {
    int slotID = m_currentSlotID++;
    m_compassStatusListeners[slotID] = l;
    return slotID;
  }

  void State::OnStartLocation()
  {
    SetCompassProcessMode(location::ECompassDoNothing);
    SetLocationProcessMode(location::ELocationCenterAndScale);
    m_isFirstPosition = true;
  }

  void State::OnStopLocation()
  {
    SetLocationProcessMode(location::ELocationDoNothing);
    SetCompassProcessMode(location::ECompassDoNothing);
    m_isFirstPosition = false;
    TurnOff();
  }

  void State::RemoveCompassStatusListener(int slotID)
  {
    m_compassStatusListeners.erase(slotID);
  }

  bool State::onTapEnded(m2::PointD const & pt)
  {
    if (!m_framework->GetNavigator().DoSupportRotation())
      return false;

    anim::Controller * controller = m_framework->GetAnimController();

    controller->Lock();

    switch (state())
    {
    case EActive:
      if (m_hasCompass)
      {
        if (!IsCentered())
          AnimateToPositionAndEnqueueFollowing();
        else
          StartCompassFollowing();
      }
      break;

    case EPressed:
      StopCompassFollowing();
      break;

    default:
      /// @todo Need to check other states?
      /// - do nothing, then write comment;
      /// - place ASSERT, if other states are impossible;
      break;
    };

    controller->Unlock();

    invalidate();
    return true;
  }
}
