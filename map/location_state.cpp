#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "compass_filter.hpp"
#include "change_viewport_task.hpp"
#include "move_screen_task.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/brush.hpp"
#include "../graphics/pen.hpp"

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
  double const State::s_cacheRadius = 500;

  State::Params::Params()
    : m_locationAreaColor(0, 0, 0, 0),
      m_compassAreaColor(0, 0, 0, 0),
      m_compassBorderColor(0, 0, 0, 0),
      m_framework(0)
  {}

  State::State(Params const & p)
    : base_t(p),
      m_errorRadius(0),
      m_position(0, 0),
      m_hasPosition(false),
      m_hasCompass(false),
      m_isCentered(false),
      m_isFirstPosition(false),
      m_locationProcessMode(ELocationDoNothing),
      m_compassProcessMode(ECompassDoNothing),
      m_currentSlotID(0)
  {
    m_drawHeading = m_compassFilter.GetHeadingRad();
    m_locationAreaColor = p.m_locationAreaColor;
    m_compassAreaColor = p.m_compassAreaColor;
    m_compassBorderColor = p.m_compassBorderColor;
    m_framework = p.m_framework;

    /// @todo Probably we can make this like static const int.
    /// It's not a class state, so no need to store it in memory.
    m_arrowScale = 0.5;
    m_arrowWidth = 40 * m_arrowScale;
    m_arrowHeight = 50 * m_arrowScale;
    m_arrowBackHeight = 11 * m_arrowScale;

    m_boundRects.resize(1);

    setColor(EActive, graphics::Color(65, 136, 210, 255));
    setColor(EPressed, graphics::Color(102, 163, 210, 255));
    setState(EActive);
    setIsVisible(false);
  }

  double State::ComputeMoveSpeed(m2::PointD const & globalPt0,
                                 m2::PointD const & globalPt1,
                                 ScreenBase const & s)
  {
    return max(0.1, min(0.5, 0.5 * s.GtoP(globalPt0).Length(s.GtoP(globalPt1)) / 50.0));
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
    m_isFirstPosition = false;

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

      // Correct rect scale if country isn't downloaded.
      int const upperScale = scales::GetUpperWorldScale();
      if (rectScale > upperScale && !m_framework->IsCountryLoaded(center))
        setScale = upperScale;
      else
      {
        // Correct scale if it's too small.
        int const bestScale = scales::GetUpperScale() - 1;
        if (rectScale > bestScale)
          setScale = bestScale;
      }

      if (setScale != -1)
      {
        /// @todo Correct rect scale for best user experience.
        /// The logic of GetRectForLevel differs from tile scale calculating logic
        /// (@see initial_level in scales.cpp).
        rect = scales::GetRectForLevel(setScale + 0.51, center, 1.0);
      }

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
    graphics::Screen * cacheScreen = m_controller->GetCacheScreen();

    shared_ptr<graphics::DisplayList> & dl = m_arrowBorderLists[state];

    dl.reset();
    dl.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(dl.get());

    double k = m_controller->GetVisualScale();

    m2::PointD ptsD[] =
    {
      m2::PointD(0, 0),
      m2::PointD(-(m_arrowWidth * k) / 2, (m_arrowBackHeight * k)),
      m2::PointD(0, -m_arrowHeight * k + m_arrowBackHeight * k),
      m2::PointD((m_arrowWidth * k) / 2, m_arrowBackHeight * k),
      m2::PointD(0, 0),
      m2::PointD(0, -m_arrowHeight * k + m_arrowBackHeight * k),
    };

    graphics::Color const borderColor = color(state);

    uint32_t penStyle = cacheScreen->mapInfo(graphics::Pen::Info(borderColor, 1 * k, 0, 0, 0));

    cacheScreen->drawPath(ptsD, ARRAY_SIZE(ptsD), 0, penStyle, depth());

    cacheScreen->setDisplayList(0);
    cacheScreen->endFrame();
  }

  void State::cacheArrowBody(EState state)
  {
    graphics::Screen * cacheScreen = m_controller->GetCacheScreen();

    shared_ptr<graphics::DisplayList> & dl = m_arrowBodyLists[state];

    dl.reset();
    dl.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(dl.get());

    double k = m_controller->GetVisualScale();

    m2::PointD pts[4] =
    {
      m2::PointD(-(m_arrowWidth * k) / 2, (m_arrowBackHeight * k)),
      m2::PointD(0, 0),
      m2::PointD(0, -m_arrowHeight * k + m_arrowBackHeight * k),
      m2::PointD((m_arrowWidth * k) / 2, m_arrowBackHeight * k),
    };

    graphics::Color const baseColor = color(state);
    graphics::Color const lightColor = graphics::Color(min(255, (baseColor.r * 5) >> 2),
                                           min(255, (baseColor.g * 5) >> 2),
                                           min(255, (baseColor.b * 5) >> 2),
                                           baseColor.a);
    cacheScreen->drawTrianglesList(&pts[0], 3,
                                   cacheScreen->mapInfo(graphics::Brush::Info(baseColor)),
                                   depth());
    cacheScreen->drawTrianglesList(&pts[1], 3,
                                   cacheScreen->mapInfo(graphics::Brush::Info(lightColor)),
                                   depth());

    cacheScreen->setDisplayList(0);
    cacheScreen->endFrame();
  }

  void State::cacheLocationMark()
  {
    graphics::Screen * cacheScreen = m_controller->GetCacheScreen();

    m_locationMarkDL.reset();
    m_locationMarkDL.reset(cacheScreen->createDisplayList());

    m_positionMarkDL.reset();
    m_positionMarkDL.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(m_locationMarkDL.get());

    cacheScreen->fillSector(m2::PointD(0, 0),
                            0, 2.0 * math::pi,
                            s_cacheRadius,
                            m_locationAreaColor,
                            depth() - 3);

    cacheScreen->setDisplayList(m_positionMarkDL.get());

    cacheScreen->drawSymbol(m2::PointD(0, 0),
                            "current-position",
                            graphics::EPosCenter,
                            depth() - 1);

    cacheScreen->setDisplayList(0);

    cacheScreen->endFrame();
  }

  void State::cache()
  {
    cacheArrowBody(EActive);
    cacheArrowBorder(EActive);
    cacheArrowBody(EPressed);
    cacheArrowBorder(EPressed);
    cacheLocationMark();

    m_controller->GetCacheScreen()->completeCommands();
  }

  void State::purge()
  {
    m_arrowBorderLists.clear();
    m_arrowBodyLists.clear();
    m_locationMarkDL.reset();
    m_positionMarkDL.reset();
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

  void State::draw(graphics::OverlayRenderer * r,
                   math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      if (m_hasPosition)
      {
        double screenAngle = m_framework->GetNavigator().Screen().GetAngle();
        math::Matrix<double, 3, 3> compassDrawM;
        math::Matrix<double, 3, 3> locationDrawM;

        /// drawing arrow body first
        if (m_hasCompass)
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

          map<EState, shared_ptr<graphics::DisplayList> >::const_iterator it;
          it = m_arrowBodyLists.find(state());

          if (it != m_arrowBodyLists.end())
            r->drawDisplayList(it->second.get(), compassDrawM * m);
          else
            LOG(LWARNING, ("m_compassDisplayLists[state()] is not set!"));
        }

        /// then position
        m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
        double const pxErrorRadius = pxPosition.Length(
              m_framework->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));

        double const drawScale = pxErrorRadius / s_cacheRadius;

        locationDrawM =
           math::Shift(
             math::Scale(
               math::Identity<double, 3>(),
               drawScale,
               drawScale),
             pivot());

        math::Matrix<double, 3, 3> const drawM = locationDrawM * m;

        if (!m_hasCompass)
          r->drawDisplayList(m_positionMarkDL.get(), drawM);

        r->drawDisplayList(m_locationMarkDL.get(), drawM);

        /// and then arrow border
        if (m_hasCompass)
        {
          map<EState, shared_ptr<graphics::DisplayList> >::const_iterator it;
          it = m_arrowBorderLists.find(state());

          if (it != m_arrowBorderLists.end())
            r->drawDisplayList(it->second.get(), compassDrawM * m);
          else
            LOG(LWARNING, ("m_arrowBorderLists[state()] is not set!"));
        }
      }
    }
  }

  bool State::roughHitTest(m2::PointD const & pt) const
  {
    return hitTest(pt);
  }

  bool State::hitTest(m2::PointD const & pt) const
  {
    double radius = m_arrowHeight * m_controller->GetVisualScale();
    return (pt.SquareLength(pivot()) <= my::sq(radius));
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

    if (floor(my::RadToDeg(headingDelta)) > 0)
      m_headingInterpolation->SetEndAngle(m_compassFilter.GetHeadingRad());
    else
    {
      if (!isRunning)
      {
        headingDelta = fabs(ang::GetShortestDistance(m_drawHeading, m_compassFilter.GetHeadingRad()));

        if (my::rounds(my::RadToDeg(headingDelta)) > 0)
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
    anim::Controller::Guard guard(controller);

    double startAngle = m_framework->GetNavigator().Screen().GetAngle();
    double endAngle = -m_compassFilter.GetHeadingRad();

    m_framework->GetAnimator().RotateScreen(startAngle, endAngle);
  }

  void State::AnimateToPosition()
  {
    anim::Controller * controller = m_framework->GetAnimController();
    anim::Controller::Guard guard(controller);

    m2::PointD startPt = m_framework->GetNavigator().Screen().GetOrg();
    m2::PointD endPt = Position();

    ScreenBase const & s = m_framework->GetNavigator().Screen();
    double speed = ComputeMoveSpeed(startPt, endPt, s);

    m_framework->GetAnimator().MoveScreen(startPt, endPt, speed);
  }

  void State::AnimateToPositionAndEnqueueFollowing()
  {
    anim::Controller * controller = m_framework->GetAnimController();
    anim::Controller::Guard guard(controller);

    m2::PointD const startPt = m_framework->GetNavigator().Screen().GetOrg();
    m2::PointD const endPt = Position();
    ScreenBase const & s = m_framework->GetNavigator().Screen();

    double const speed = ComputeMoveSpeed(startPt, endPt, s);

    shared_ptr<MoveScreenTask> const & t = m_framework->GetAnimator().MoveScreen(startPt, endPt, speed);

    t->Lock();
    t->AddCallback(anim::Task::EEnded, bind(&State::SetIsCentered, this, true));
    t->AddCallback(anim::Task::EEnded, bind(&State::StartCompassFollowing, this));
    t->Unlock();
  }

  void State::AnimateToPositionAndEnqueueLocationProcessMode(location::ELocationProcessMode mode)
  {
    anim::Controller * controller = m_framework->GetAnimController();
    anim::Controller::Guard guard(controller);

    m2::PointD const startPt = m_framework->GetNavigator().Screen().GetOrg();
    m2::PointD const endPt = Position();

    ScreenBase const & s = m_framework->GetNavigator().Screen();
    double const speed = ComputeMoveSpeed(startPt, endPt, s);

    shared_ptr<MoveScreenTask> const & t = m_framework->GetAnimator().MoveScreen(startPt, endPt, speed);

    t->Lock();
    t->AddCallback(anim::Task::EEnded, bind(&State::SetIsCentered, this, true));
    t->AddCallback(anim::Task::EEnded, bind(&State::SetLocationProcessMode, this, mode));
    t->Unlock();
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
    m_framework->GetAnimator().StopMoveScreen();
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
    CallOnPositionClickListeners(pt);
    return false;
//    if (!m_framework->GetNavigator().DoSupportRotation())
//      return false;

//    anim::Controller::Guard guard(m_framework->GetAnimController());

//    switch (state())
//    {
//    case EActive:
//      if (m_hasCompass)
//      {
//        if (!IsCentered())
//          AnimateToPositionAndEnqueueFollowing();
//        else
//          StartCompassFollowing();
//      }
//      break;

//    case EPressed:
//      StopCompassFollowing();
//      break;

//    default:
//      /// @todo Need to check other states?
//      /// - do nothing, then write comment;
//      /// - place ASSERT, if other states are impossible;
//      break;
//    };

//    invalidate();
//    return true;
  }

  void State::CallOnPositionClickListeners(m2::PointD const & point)
  {
    for (TOnPositionClickListeners::const_iterator it = m_onPositionClickListeners.begin();
         it != m_onPositionClickListeners.end();
         ++it)
      it->second(Position());
  }

  int State::AddOnPositionClickListener(TOnPositionClickListener const & listner)
  {
    int slotID = m_currentSlotID++;
    m_onPositionClickListeners[slotID] = listner;
    return slotID;
  }

  void State::RemoveOnPositionClickListener(int listnerID)
  {
    m_onPositionClickListeners.erase(listnerID);
  }
}
