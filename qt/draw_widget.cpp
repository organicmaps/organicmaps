#include "qt/draw_widget.hpp"

#include "qt/slider_ctrl.hpp"

#include "drape_frontend/visual_params.hpp"

#include "search/result.hpp"

#include "platform/settings.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QGuiApplication>

#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <QtCore/QLocale>
#include <QtCore/QDateTime>

namespace qt
{

namespace
{

bool IsLeftButton(Qt::MouseButtons buttons)
{
  return buttons & Qt::LeftButton;
}

bool IsLeftButton(QMouseEvent * e)
{
  return IsLeftButton(e->button()) || IsLeftButton(e->buttons());
}

bool IsRightButton(Qt::MouseButtons buttons)
{
  return buttons & Qt::RightButton;
}

bool IsRightButton(QMouseEvent * e)
{
  return IsRightButton(e->button()) || IsRightButton(e->buttons());
}

bool IsRotation(QMouseEvent * e)
{
  return e->modifiers() & Qt::ControlModifier;
}

bool IsRouting(QMouseEvent * e)
{
  return e->modifiers() & Qt::ShiftModifier;
}

bool IsLocationEmulation(QMouseEvent * e)
{
  return e->modifiers() & Qt::AltModifier;
}

} // namespace

  void DummyDismiss() {}

  DrawWidget::DrawWidget()
    : m_contextFactory(nullptr),
      m_framework(new Framework()),
      m_ratio(1.0),
      m_pScale(nullptr),
      m_emulatingLocation(false),
      m_enableScaleUpdate(true)
  {
    setSurfaceType(QSurface::OpenGLSurface);

    QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(sizeChanged(int)));
    QObject::connect(this, SIGNAL(widthChanged(int)), this,  SLOT(sizeChanged(int)));

    // Initialize with some stubs for test.
    PinClickManager & manager = GetBalloonManager();
    manager.ConnectUserMarkListener(bind(&DrawWidget::OnActivateMark, this, _1));
    manager.ConnectDismissListener(&DummyDismiss);

    m_framework->SetRouteBuildingListener([] (routing::IRouter::ResultCode, vector<storage::TIndex> const &)
    {
    });
  }

  DrawWidget::~DrawWidget()
  {
    m_framework.reset();
  }

  void DrawWidget::SetScaleControl(QScaleSlider * pScale)
  {
    m_pScale = pScale;

    connect(m_pScale, SIGNAL(actionTriggered(int)), this, SLOT(ScaleChanged(int)));
    connect(m_pScale, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
    connect(m_pScale, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
  }

  void DrawWidget::PrepareShutdown()
  {
    m_framework->PrepareToShutdown();
    m_contextFactory.reset();
  }

  void DrawWidget::UpdateAfterSettingsChanged()
  {
    m_framework->SetupMeasurementSystem();
  }

  void DrawWidget::LoadState()
  {
    m_framework->LoadState();
    m_framework->LoadBookmarks();
  }

  void DrawWidget::SaveState()
  {
    m_framework->SaveState();
  }

  void DrawWidget::ScalePlus()
  {
    m_framework->Scale(Framework::SCALE_MAG, true);
  }

  void DrawWidget::ScaleMinus()
  {
    m_framework->Scale(Framework::SCALE_MIN, true);
  }

  void DrawWidget::ScalePlusLight()
  {
    m_framework->Scale(Framework::SCALE_MAG_LIGHT, true);
  }

  void DrawWidget::ScaleMinusLight()
  {
    m_framework->Scale(Framework::SCALE_MIN_LIGHT, true);
  }

  void DrawWidget::ShowAll()
  {
    m_framework->ShowAll();
  }

  void DrawWidget::ScaleChanged(int action)
  {
    if (action != QAbstractSlider::SliderNoAction)
    {
      double const factor = m_pScale->GetScaleFactor();
      if (factor != 1.0)
        m_framework->Scale(factor, false);
    }
  }

  void DrawWidget::SliderPressed()
  {
    m_enableScaleUpdate = false;
  }

  void DrawWidget::SliderReleased()
  {
    m_enableScaleUpdate = true;
  }

  void DrawWidget::OnActivateMark(unique_ptr<UserMarkCopy> pCopy)
  {
    UserMark const * pMark = pCopy->GetUserMark();
    m_framework->ActivateUserMark(pMark);
  }

  void DrawWidget::CreateEngine()
  {
    m_framework->CreateDrapeEngine(make_ref(m_contextFactory), m_ratio,
                                   m_ratio * width(), m_ratio * height());
    m_framework->AddViewportListener(bind(&DrawWidget::OnViewportChanged, this, _1));
  }

  void DrawWidget::exposeEvent(QExposeEvent * e)
  {
    Q_UNUSED(e);

    if (isExposed())
    {
      if (m_contextFactory == nullptr)
      {
        m_ratio = devicePixelRatio();
        m_contextFactory = make_unique_dp<dp::ThreadSafeFactory>(new QtOGLContextFactory(this));
        CreateEngine();
        LoadState();

        emit EngineCreated();
      }
    }
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    TBase::mousePressEvent(e);

    m2::PointD const pt = GetDevicePoint(e);

    if (IsLeftButton(e))
    {
      if (IsRouting(e))
        SubmitRoutingPoint(pt);
      else if (IsLocationEmulation(e))
        SubmitFakeLocationPoint(pt);
      else
      {
        m_framework->TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_DOWN));
        setCursor(Qt::CrossCursor);
      }
    }
    else if (IsRightButton(e))
      ShowInfoPopup(pt);
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    TBase::mouseDoubleClickEvent(e);
    if (IsLeftButton(e))
      m_framework->Scale(Framework::SCALE_MAG_LIGHT, GetDevicePoint(e), true);
  }

  void DrawWidget::mouseMoveEvent(QMouseEvent * e)
  {
    TBase::mouseMoveEvent(e);
    if (IsLeftButton(e) && !IsLocationEmulation(e))
      m_framework->TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_MOVE));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    TBase::mouseReleaseEvent(e);
    if (IsLeftButton(e) && !IsLocationEmulation(e))
      m_framework->TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_UP));
  }

  void DrawWidget::keyPressEvent(QKeyEvent * e)
  {
    TBase::keyPressEvent(e);
    if (IsLeftButton(QGuiApplication::mouseButtons()) &&
        e->key() == Qt::Key_Control)
    {
      df::TouchEvent event;
      event.m_type = df::TouchEvent::TOUCH_DOWN;
      event.m_touches[0].m_id = 0;
      event.m_touches[0].m_location = m2::PointD(L2D(QCursor::pos().x()), L2D(QCursor::pos().y()));
      event.m_touches[1] = GetSymmetrical(event.m_touches[0]);

      m_framework->TouchEvent(event);
    }
  }

  void DrawWidget::keyReleaseEvent(QKeyEvent * e)
  {
    TBase::keyReleaseEvent(e);

    if (IsLeftButton(QGuiApplication::mouseButtons()) &&
        e->key() == Qt::Key_Control)
    {
      df::TouchEvent event;
      event.m_type = df::TouchEvent::TOUCH_UP;
      event.m_touches[0].m_id = 0;
      event.m_touches[0].m_location = m2::PointD(L2D(QCursor::pos().x()), L2D(QCursor::pos().y()));
      event.m_touches[1] = GetSymmetrical(event.m_touches[0]);

      m_framework->TouchEvent(event);
    }
    else if (e->key() == Qt::Key_Alt)
      m_emulatingLocation = false;
  }

  void DrawWidget::wheelEvent(QWheelEvent * e)
  {
    m_framework->Scale(exp(e->delta() / 360.0), m2::PointD(L2D(e->x()), L2D(e->y())), false);
  }

  bool DrawWidget::Search(search::SearchParams params)
  {
    double lat, lon;
    if (m_framework->GetCurrentPosition(lat, lon))
      params.SetPosition(lat, lon);

    return m_framework->Search(params);
  }

  string DrawWidget::GetDistance(search::Result const & res) const
  {
    string dist;
    double lat, lon;
    if (m_framework->GetCurrentPosition(lat, lon))
    {
      double dummy;
      (void) m_framework->GetDistanceAndAzimut(res.GetFeatureCenter(), lat, lon, -1.0, dist, dummy);
    }
    return dist;
  }

  void DrawWidget::ShowSearchResult(search::Result const & res)
  {
    m_framework->ShowSearchResult(res);
  }

  void DrawWidget::OnLocationUpdate(location::GpsInfo const & info)
  {
    if (!m_emulatingLocation)
      m_framework->OnLocationUpdate(info);
  }

  void DrawWidget::SetMapStyle(MapStyle mapStyle)
  {
    //@TODO UVR
  }

  void DrawWidget::sizeChanged(int)
  {
    m_framework->OnSize(m_ratio * width(), m_ratio * height());
  }

  void DrawWidget::SubmitFakeLocationPoint(m2::PointD const & pt)
  {
    m_emulatingLocation = true;
    m2::PointD const point = m_framework->PtoG(pt);

    location::GpsInfo info;
    info.m_latitude = MercatorBounds::YToLat(point.y);
    info.m_longitude = MercatorBounds::XToLon(point.x);
    info.m_horizontalAccuracy = 10;
    info.m_timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    m_framework->OnLocationUpdate(info);

    if (m_framework->IsRoutingActive())
    {
      location::FollowingInfo loc;
      m_framework->GetRouteFollowingInfo(loc);
      LOG(LDEBUG, ("Distance:", loc.m_distToTarget, loc.m_targetUnitsSuffix, "Time:", loc.m_time,
                   "Turn:", routing::turns::GetTurnString(loc.m_turn), "(", loc.m_distToTurn, loc.m_turnUnitsSuffix,
                   ") Roundabout exit number:", loc.m_exitNum));
    }
  }

  void DrawWidget::SubmitRoutingPoint(m2::PointD const & pt)
  {
    if (m_framework->IsRoutingActive())
      m_framework->CloseRouting();
    else
      m_framework->BuildRoute(m_framework->PtoG(pt));
  }

  void DrawWidget::ShowInfoPopup(m2::PointD const & pt)
  {
    // show feature types
    QMenu menu;
    auto const addStringFn = [&menu](string const & s)
    {
      menu.addAction(QString::fromUtf8(s.c_str()));
    };

    // Get POI under cursor or nearest address by point.
    m2::PointD dummy;
    search::AddressInfo info;
    feature::FeatureMetadata metadata;
    if (m_framework->GetVisiblePOI(pt, dummy, info, metadata))
      addStringFn("POI");
    else
      m_framework->GetAddressInfoForPixelPoint(pt, info);

    // Get feature types under cursor.
    vector<string> types;
    m_framework->GetFeatureTypes(pt, types);
    for (size_t i = 0; i < types.size(); ++i)
      addStringFn(types[i]);

    menu.addSeparator();

    // Format address and types.
    if (!info.m_name.empty())
      addStringFn(info.m_name);
    addStringFn(info.FormatAddress());
    addStringFn(info.FormatTypes());

    menu.exec();
  }

  void DrawWidget::OnViewportChanged(ScreenBase const & screen)
  {
    UpdateScaleControl();
  }

  void DrawWidget::UpdateScaleControl()
  {
    if (m_pScale && isExposed() && m_enableScaleUpdate)
    {
      // don't send ScaleChanged
      m_pScale->SetPosWithBlockedSignals(m_framework->GetDrawScale());
    }
  }

  df::Touch DrawWidget::GetTouch(QMouseEvent * e)
  {
    df::Touch touch;
    touch.m_id = 0;
    touch.m_location = GetDevicePoint(e);
    return touch;
  }

  df::Touch DrawWidget::GetSymmetrical(df::Touch const & touch)
  {
    m2::PointD pixelCenter = m_framework->GetPixelCenter();
    m2::PointD symmetricalLocation = pixelCenter + (pixelCenter - touch.m_location);

    df::Touch result;
    result.m_id = touch.m_id + 1;
    result.m_location = symmetricalLocation;

    return result;
  }

  df::TouchEvent DrawWidget::GetTouchEvent(QMouseEvent * e, df::TouchEvent::ETouchType type)
  {
    df::TouchEvent event;
    event.m_type = type;
    event.m_touches[0] = GetTouch(e);
    if (IsRotation(e))
      event.m_touches[1] = GetSymmetrical(event.m_touches[0]);

    return event;
  }

  m2::PointD DrawWidget::GetDevicePoint(QMouseEvent * e) const
  {
    return m2::PointD(L2D(e->x()), L2D(e->y()));
  }

  void DrawWidget::SetRouter(routing::RouterType routerType)
  {
    m_framework->SetRouter(routerType);
  }
}
