#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "move_screen_task.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"

#include "../anim/controller.hpp"
#include "../anim/task.hpp"

#include "../gui/controller.hpp"

#include "../indexer/mercator.hpp"

#include "../platform/platform.hpp"
#include "../platform/location.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/transformations.hpp"


namespace location
{
//  namespace
//  {
//    const float MaxPositionFault = 25.0;
//    const float MaxHeadingFaultDeg = 3.0;
//  }

  double const State::s_cacheRadius = 500;

  State::Params::Params()
    : m_locationAreaColor(0, 0, 0, 0),
      m_framework(0)
  {}

  State::State(Params const & p)
    : BaseT(p),
      m_errorRadius(0),
      m_position(0, 0),
      m_drawHeading(0.0),
      m_hasPosition(false),
      m_positionFault(0.0),
      m_hasCompass(false),
      m_compassFault(0.0),
      m_isCentered(false),
      m_isFirstPosition(false),
      m_currentSlotID(0),
      m_locationProcessMode(ELocationDoNothing),
      m_compassProcessMode(ECompassDoNothing)
  {
    m_locationAreaColor = p.m_locationAreaColor;
    m_framework = p.m_framework;

    m_boundRects.resize(1);

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
    return m_hasCompass && !IsPositionFaultCritical() && !IsCompassFaultCritical();
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
    m_positionFault = info.m_horizontalAccuracy;

    m2::RectD rect = MercatorBounds::MetresToXY(info.m_longitude,
                                                info.m_latitude,
                                                m_positionFault);
    m2::PointD const center = rect.Center();

    m_hasPosition = true;
    setIsVisible(true);
    m_position = center;
    m_errorRadius = rect.SizeX() / 2;

    switch (m_locationProcessMode)
    {
    case ELocationCenterAndScale:
      m_framework->ShowRectExVisibleScale(rect, scales::GetUpperComfortScale());

      SetIsCentered(true);
      CheckCompassFollowing();

      m_locationProcessMode = ELocationCenterOnly;
      break;

    case ELocationCenterOnly:
      m_framework->SetViewportCenter(center);

      SetIsCentered(true);
      CheckCompassFollowing();
      break;

    case ELocationDoNothing:
      break;
    }

    CallPositionChangedListeners(m_position);
    invalidate();
  }

  void State::OnCompassUpdate(location::CompassInfo const & info)
  {
    m_hasCompass = true;
    m_compassFault = info.m_accuracy;
    if (info.m_trueHeading >= 0.0)
    {
      m_compassFault = 0.0;
      m_drawHeading = info.m_trueHeading;
    }
    else
      m_drawHeading = info.m_magneticHeading;

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

  bool State::IsPositionFaultCritical() const
  {
    //return m_positionFault > MaxPositionFault;
    return false;
  }

  bool State::IsCompassFaultCritical() const
  {
    //static double s_maxOffset = my::DegToRad(MaxHeadingFaultDeg);
    //return m_compassFault > s_maxOffset;
    return false;
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

  void State::RemoveCompassStatusListener(int slotID)
  {
    m_compassStatusListeners.erase(slotID);
  }

  void State::CallPositionChangedListeners(m2::PointD const & pt)
  {
    typedef TPositionChangedListeners::const_iterator iter_t;
    for (iter_t it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
      it->second(pt);
  }

  int State::AddPositionChangedListener(State::TPositionChangedCallback const & func)
  {
    int result = m_currentSlotID++;
    m_callbacks[result] = func;
    return result;
  }

  void State::RemovePositionChangedListener(int slotID)
  {
    m_callbacks.erase(slotID);
  }

  void State::cachePositionArrow()
  {
    graphics::Screen * cacheScreen = m_controller->GetCacheScreen();
    graphics::Icon::Info info("current-position-compas");

    graphics::Resource const * res = cacheScreen->fromID(cacheScreen->findInfo(info));
    m2::RectU rect = res->m_texRect;
    m_halfArrowSize.x = rect.SizeX() / 2.0;
    m_halfArrowSize.y = rect.SizeY() / 2.0;

    m_positionArrow.reset();
    m_positionArrow.reset(cacheScreen->createDisplayList());

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(m_positionArrow.get());

    m2::PointD coords[4] =
    {
      m2::PointD(-m_halfArrowSize.x, -m_halfArrowSize.y),
      m2::PointD(-m_halfArrowSize.x,  m_halfArrowSize.y),
      m2::PointD( m_halfArrowSize.x, -m_halfArrowSize.y),
      m2::PointD( m_halfArrowSize.x,  m_halfArrowSize.y)
    };

    m2::PointF normal(0.0, 0.0);
    shared_ptr<graphics::gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();

    m2::PointF texCoords[4] =
    {
      texture->mapPixel(m2::PointF(rect.minX(), rect.minY())),
      texture->mapPixel(m2::PointF(rect.minX(), rect.maxY())),
      texture->mapPixel(m2::PointF(rect.maxX(), rect.minY())),
      texture->mapPixel(m2::PointF(rect.maxX(), rect.maxY()))
    };

    cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
                                         &normal, 0,
                                         texCoords, sizeof(m2::PointF),
                                         4, depth(), res->m_pipelineID);
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
    cachePositionArrow();
    cacheLocationMark();

    m_controller->GetCacheScreen()->completeCommands();
  }

  void State::purge()
  {
    m_positionArrow.reset();
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

      m2::RectD arrowRect(pxPosition - m_halfArrowSize,
                          pxPosition + m_halfArrowSize);

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
        math::Matrix<double, 3, 3> locationDrawM;

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
        r->drawDisplayList(m_locationMarkDL.get(), drawM);

        if (HasCompass())
        {
          double screenAngle = m_framework->GetNavigator().Screen().GetAngle();

          math::Matrix<double, 3, 3> compassDrawM;

          double const headingRad = m_drawHeading;

          compassDrawM =
              math::Shift(
                math::Rotate(
                    math::Identity<double, 3>(),
                    screenAngle + headingRad),
                pivot());

          r->drawDisplayList(m_positionArrow.get(), compassDrawM * m);
        }
        else
          r->drawDisplayList(m_positionMarkDL.get(), drawM);
      }
    }
  }

  bool State::roughHitTest(m2::PointD const & pt) const
  {
    return hitTest(pt);
  }

  bool State::hitTest(m2::PointD const & pt) const
  {
    return false;
  }

  void State::CheckCompassFollowing()
  {
    if (HasCompass()
    && (GetCompassProcessMode() == ECompassFollow)
    && IsCentered())
      FollowCompass();
  }

  void State::FollowCompass()
  {
    if (!m_framework->GetNavigator().DoSupportRotation())
      return;

    anim::Controller::Guard guard(m_framework->GetAnimController());

    m_framework->GetAnimator().RotateScreen(
          m_framework->GetNavigator().Screen().GetAngle(),
          -m_drawHeading);
  }

  void State::AnimateToPosition()
  {
    m_framework->SetViewportCenterAnimated(Position());
  }

  void State::AnimateToPositionAndEnqueueFollowing()
  {
    shared_ptr<MoveScreenTask> const & t = m_framework->SetViewportCenterAnimated(Position());

    t->Lock();
    t->AddCallback(anim::Task::EEnded, bind(&State::SetIsCentered, this, true));
    t->AddCallback(anim::Task::EEnded, bind(&State::StartCompassFollowing, this));
    t->Unlock();
  }

  void State::AnimateToPositionAndEnqueueLocationProcessMode(location::ELocationProcessMode mode)
  {
    shared_ptr<MoveScreenTask> const & t = m_framework->SetViewportCenterAnimated(Position());

    t->Lock();
    t->AddCallback(anim::Task::EEnded, bind(&State::SetIsCentered, this, true));
    t->AddCallback(anim::Task::EEnded, bind(&State::SetLocationProcessMode, this, mode));
    t->Unlock();
  }

  void State::StartCompassFollowing()
  {
    SetCompassProcessMode(ECompassFollow);
    SetLocationProcessMode(ELocationCenterOnly);

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
}
