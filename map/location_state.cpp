#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "move_screen_task.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"

#include "../anim/controller.hpp"
#include "../anim/task.hpp"

#include "../gui/controller.hpp"

#include "../platform/platform.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/transformations.hpp"

#include "../indexer/mercator.hpp"

#include "../base/logging.hpp"

namespace location
{
  namespace
  {
    class ErrorSectorAnimator : public anim::Task
    {
      typedef anim::Task base_t;
    public:
      ErrorSectorAnimator(double maxRadius, ::location::State * state)
        : m_maxRadius(maxRadius)
        , m_currentRadius(0.0)
        , m_pause(0.0)
        , m_state(state)
      {
      }

      void Update(Framework * f)
      {
        ScreenBase s = f->GetNavigator().Screen();
        m2::PointD pxPosition = m_state->pivot();
        double pxRadius = pxPosition.Length(s.GtoP(m_state->Position() + m2::PointD(m_maxRadius, 0)));

        m2::RectD r = s.PixelRect();
        double minSize = min(r.SizeX(), r.SizeY());
        double factor = pxRadius / minSize;

        double percent = 1E-2;
        if (factor > 1.0)
          percent = 5E-3;
        else if (factor > 0.1)
          percent = 9E-3;

        m_baseVelocity = percent * m_maxRadius;
      }

      void SetMaxRadius(double maxRadius)
      {
        m_maxRadius = maxRadius;
      }

      double GetCurrentRadius() const
      {
        return m_currentRadius;
      }

      float GetTransparency() const
      {
        return 0.3 * (1 - m_currentRadius / m_maxRadius);
      }

      virtual void OnStart(double ts)
      {
        m_startTime = ts;
      }

      virtual void OnStep(double ts)
      {
        base_t::OnStep(ts);

        double time = ts - (m_startTime + m_pause);
        if (time > 0.0)
        {
          double e = exp(-time) + 0.5;
          m_currentRadius += (max(e, 0.3) * m_baseVelocity);

          if (m_currentRadius > m_maxRadius)
          {
            m_currentRadius = 0.0;
            m_startTime = ts;
            m_pause = 0.5;
          }
        }

        m_state->invalidate();
      }

    private:
      double m_maxRadius;
      double m_currentRadius;
      double m_startTime;
      double m_pause;
      double m_baseVelocity;
      location::State * m_state;
    };
  }

  double const State::s_cacheRadius = 500;

  State::Params::Params()
    : m_locationAreaColor(0, 0, 0, 0),
      m_framework(0)
  {}

  State::State(Params const & p)
    : base_t(p),
      m_errorRadius(0),
      m_position(0, 0),
      m_drawHeading(0.0),
      m_hasPosition(false),
      m_hasCompass(false),
      m_isCentered(false),
      m_isFirstPosition(false),
      m_locationProcessMode(ELocationDoNothing),
      m_compassProcessMode(ECompassDoNothing),
      m_currentSlotID(0)
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

  void State::setIsVisible(bool isVisible)
  {
    gui::Element::setIsVisible(isVisible);
    if (isVisible)
    {
      if (m_radiusAnimation == NULL)
      {
        m_radiusAnimation.reset(new ErrorSectorAnimator(m_errorRadius, this));
        m_framework->GetAnimController()->AddTask(m_radiusAnimation);
      }
    }
    else
    {
      if (m_radiusAnimation)
      {
        m_radiusAnimation->End();
        m_radiusAnimation.reset();
      }
    }
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
    SetErrorRadius(m_errorRadius);

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

    invalidate();
  }

  void State::OnCompassUpdate(location::CompassInfo const & info)
  {
    m_hasCompass = true;

    m_drawHeading =
        ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);

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

  void State::UpdateAnimation()
  {
    if (m_radiusAnimation)
    {
      ErrorSectorAnimator * a = static_cast<ErrorSectorAnimator *>(m_radiusAnimation.get());
      a->Update(m_framework);
    }
  }

  void State::SetErrorRadius(double errorRadius)
  {
    if (m_radiusAnimation)
    {
      ErrorSectorAnimator * a = static_cast<ErrorSectorAnimator *>(m_radiusAnimation.get());
      a->SetMaxRadius(errorRadius);
    }
  }

  double State::GetErrorRadius() const
  {
    if (m_radiusAnimation)
    {
      ErrorSectorAnimator * a = static_cast<ErrorSectorAnimator *>(m_radiusAnimation.get());
      return a->GetCurrentRadius();
    }

    return 0.0;
  }

  float State::GetTransparency() const
  {
    if (m_radiusAnimation)
    {
      ErrorSectorAnimator * a = static_cast<ErrorSectorAnimator *>(m_radiusAnimation.get());
      return a->GetTransparency();
    }

    return m_locationAreaColor.a;
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
    cacheScreen->applyVarAlfaStates();

    cacheScreen->fillSector(m2::PointD(0, 0),
                            0, 2.0 * math::pi,
                            s_cacheRadius,
                            m_locationAreaColor,
                            depth() - 3);

    cacheScreen->applyStates();
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

      UpdateAnimation();
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
              m_framework->GetNavigator().GtoP(Position() + m2::PointD(GetErrorRadius(), 0.0)));

        double const drawScale = pxErrorRadius / s_cacheRadius;

        locationDrawM =
           math::Shift(
             math::Scale(
               math::Identity<double, 3>(),
               drawScale,
               drawScale),
             pivot());

        math::Matrix<double, 3, 3> const drawM = locationDrawM * m;

        graphics::UniformsHolder holder;
        holder.insertValue(graphics::ETransparency, GetTransparency());
        r->drawDisplayList(m_locationMarkDL.get(), drawM, &holder);

        if (m_hasCompass)
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
    double radius = m_halfArrowSize.x;
    return (pt.SquareLength(pivot()) <= my::sq(radius));
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
