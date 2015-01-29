#include "qt/draw_widget.hpp"

#include "qt/slider_ctrl.hpp"

#include "map/country_status_display.hpp"
#include "drape_frontend/visual_params.hpp"

#include "render/render_policy.hpp"
#include "render/frame_image.hpp"

#include "search/result.hpp"

#include "platform/settings.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "std/chrono.hpp"

#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtGui/QMouseEvent>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QApplication>
  #include <QtGui/QDesktopWidget>
  #include <QtGui/QMenu>
#else
  #include <QtWidgets/QApplication>
  #include <QtWidgets/QDesktopWidget>
  #include <QtWidgets/QMenu>
#endif

namespace qt
{
  const unsigned LONG_TOUCH_MS = 1000;
  const unsigned SHORT_TOUCH_MS = 250;

  void DummyDismiss() {}

  DrawWidget::DrawWidget(QWidget * pParent)
    : m_contextFactory(nullptr),
      m_framework(new Framework()),
      m_isDrag(false),
      m_isRotate(false),
      m_ratio(1.0),
      m_pScale(0),
      m_emulatingLocation(false)
  {
    setSurfaceType(QSurface::OpenGLSurface);

    QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(sizeChanged(int)));
    QObject::connect(this, SIGNAL(widthChanged(int)), this,  SLOT(sizeChanged(int)));

    // Initialize with some stubs for test.
    PinClickManager & manager = GetBalloonManager();
    manager.ConnectUserMarkListener(bind(&DrawWidget::OnActivateMark, this, _1));
    manager.ConnectDismissListener(&DummyDismiss);

    ///@TODO UVR
    //m_framework->GetCountryStatusDisplay()->SetDownloadCountryListener([this] (storage::TIndex const & idx, int opt)
    //{
    // storage::ActiveMapsLayout & layout = m_framework->GetCountryTree().GetActiveMapLayout();
    //  if (opt == -1)
    //    layout.RetryDownloading(idx);
    //  else
    //    layout.DownloadMap(idx, static_cast<storage::TMapOptions>(opt));
    //});

    //m_framework->SetRouteBuildingListener([] (routing::IRouter::ResultCode, vector<storage::TIndex> const &)
    //{
    //});
  }

  DrawWidget::~DrawWidget()
  {
    m_framework.reset();
  }

  void DrawWidget::PrepareShutdown()
  {
    KillPressTask();

    m_framework->PrepareToShutdown();
    m_contextFactory.Destroy();
  }

  void DrawWidget::SetScaleControl(QScaleSlider * pScale)
  {
    m_pScale = pScale;

    connect(m_pScale, SIGNAL(actionTriggered(int)), this, SLOT(ScaleChanged(int)));
  }

  void DrawWidget::UpdateAfterSettingsChanged()
  {
    m_framework->SetupMeasurementSystem();
  }

  void DrawWidget::LoadState()
  {
    m_framework->LoadBookmarks();

    if (!m_framework->LoadState())
      ShowAll();
    else
      UpdateScaleControl();
  }

  void DrawWidget::SaveState()
  {
    m_framework->SaveState();
  }

  void DrawWidget::MoveLeft()
  {
    m_framework->Move(math::pi, 0.5);
  }

  void DrawWidget::MoveRight()
  {
    m_framework->Move(0.0, 0.5);
  }

  void DrawWidget::MoveUp()
  {
    m_framework->Move(math::pi/2.0, 0.5);
  }

  void DrawWidget::MoveDown()
  {
    m_framework->Move(-math::pi/2.0, 0.5);
  }

  void DrawWidget::ScalePlus()
  {
    m_framework->Scale(2.0);
    UpdateScaleControl();
  }

  void DrawWidget::ScaleMinus()
  {
    m_framework->Scale(0.5);
    UpdateScaleControl();
  }

  void DrawWidget::ScalePlusLight()
  {
    m_framework->Scale(1.5);
    UpdateScaleControl();
  }

  void DrawWidget::ScaleMinusLight()
  {
    m_framework->Scale(2.0/3.0);
    UpdateScaleControl();
  }

  void DrawWidget::ShowAll()
  {
    m_framework->ShowAll();
    UpdateScaleControl();
  }

  void DrawWidget::ScaleChanged(int action)
  {
    if (action != QAbstractSlider::SliderNoAction)
    {
      double const factor = m_pScale->GetScaleFactor();
      if (factor != 1.0)
        m_framework->Scale(factor);
    }
  }

  void DrawWidget::StartPressTask(m2::PointD const & pt, unsigned ms)
  {
    KillPressTask();
    m_deferredTask.reset(
        new DeferredTask(bind(&DrawWidget::OnPressTaskEvent, this, pt, ms), milliseconds(ms)));
  }

  void DrawWidget::KillPressTask() { m_deferredTask.reset(); }

  void DrawWidget::OnPressTaskEvent(m2::PointD const & pt, unsigned ms)
  {
    m_wasLongClick = (ms == LONG_TOUCH_MS);
    GetBalloonManager().OnShowMark(m_framework->GetUserMark(pt, m_wasLongClick));
  }

  void DrawWidget::OnActivateMark(unique_ptr<UserMarkCopy> pCopy)
  {
    UserMark const * pMark = pCopy->GetUserMark();
    m_framework->ActivateUserMark(pMark);
  }

  void DrawWidget::CreateEngine()
  {
    m_framework->CreateDrapeEngine(m_contextFactory.GetRefPointer(), m_ratio, width(), height());
  }

  void DrawWidget::exposeEvent(QExposeEvent * e)
  {
    Q_UNUSED(e);

    if (isExposed())
    {
      if (m_contextFactory.IsNull())
      {
        m_ratio = devicePixelRatio();
        dp::ThreadSafeFactory * factory = new dp::ThreadSafeFactory(new QtOGLContextFactory(this));
        m_contextFactory = dp::MasterPointer<dp::OGLContextFactory>(factory);
        CreateEngine();
        LoadState();
        UpdateScaleControl();
      }
    }
  }

  m2::PointD DrawWidget::GetDevicePoint(QMouseEvent * e) const
  {
    return m2::PointD(L2D(e->x()), L2D(e->y()));
  }

  DragEvent DrawWidget::GetDragEvent(QMouseEvent * e) const
  {
    return DragEvent(L2D(e->x()), L2D(e->y()));
  }

  RotateEvent DrawWidget::GetRotateEvent(QPoint const & pt) const
  {
    QPoint const center = geometry().center();
    return RotateEvent(L2D(center.x()), L2D(center.y()),
                       L2D(pt.x()), L2D(pt.y()));
  }

  namespace
  {
    void add_string(QMenu & menu, string const & s)
    {
      (void)menu.addAction(QString::fromUtf8(s.c_str()));
    }
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    TBase::mousePressEvent(e);

    KillPressTask();

    m2::PointD const pt = GetDevicePoint(e);

    if (e->button() == Qt::LeftButton)
    {
      ///@TODO UVR
//      if (m_framework->GetGuiController()->OnTapStarted(pt))
//        return;

      if (e->modifiers() & Qt::ControlModifier)
      {
        // starting rotation
        m_framework->StartRotate(GetRotateEvent(e->pos()));

        setCursor(Qt::CrossCursor);
        m_isRotate = true;
      }
      else if (e->modifiers() & Qt::ShiftModifier)
      {
        if (m_framework->IsRoutingActive())
          m_framework->CloseRouting();
        else
        {
          auto const & state = m_framework->GetLocationState();
          if (state->IsModeHasPosition())
            m_framework->BuildRoute(state->Position(), m_framework->PtoG(pt), 0 /* timeoutSec */);
          else
            return;
        }
      }
      if (e->modifiers() & Qt::AltModifier)
      {
        m_emulatingLocation = true;
        m2::PointD const point = m_framework->PtoG(pt);

        location::GpsInfo info;
        info.m_latitude = MercatorBounds::YToLat(point.y);
        info.m_longitude = MercatorBounds::XToLon(point.x);
        info.m_horizontalAccuracy = 10.0;
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
      else
      {
        // init press task params
        m_taskPoint = pt;
        m_wasLongClick = false;
        m_isCleanSingleClick = true;

        StartPressTask(pt, LONG_TOUCH_MS);

        // starting drag
        m_framework->StartDrag(GetDragEvent(e));

        setCursor(Qt::CrossCursor);
        m_isDrag = true;
      }
    }
    else if (e->button() == Qt::RightButton)
    {
      // show feature types
      QMenu menu;

      // Get POI under cursor or nearest address by point.
      m2::PointD dummy;
      search::AddressInfo info;
      feature::Metadata metadata;
      if (m_framework->GetVisiblePOI(pt, dummy, info, metadata))
        add_string(menu, "POI");
      else
        m_framework->GetAddressInfoForPixelPoint(pt, info);

      // Get feature types under cursor.
      vector<string> types;
      m_framework->GetFeatureTypes(pt, types);
      for (size_t i = 0; i < types.size(); ++i)
        add_string(menu, types[i]);

      (void)menu.addSeparator();

      // Format address and types.
      if (!info.m_name.empty())
        add_string(menu, info.m_name);
      add_string(menu, info.FormatAddress());
      add_string(menu, info.FormatTypes());

      menu.exec(e->pos());
    }
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    TBase::mouseDoubleClickEvent(e);

    KillPressTask();
    m_isCleanSingleClick = false;

    if (e->button() == Qt::LeftButton)
    {
      StopDragging(e);

      m_framework->ScaleToPoint(ScaleToPointEvent(L2D(e->x()), L2D(e->y()), 1.5));

      UpdateScaleControl();
    }
  }

  void DrawWidget::mouseMoveEvent(QMouseEvent * e)
  {
    TBase::mouseMoveEvent(e);

    m2::PointD const pt = GetDevicePoint(e);
    if (!pt.EqualDxDy(m_taskPoint, df::VisualParams::Instance().GetVisualScale() * 10.0))
    {
      // moved far from start point - do not show balloon
      m_isCleanSingleClick = false;
      KillPressTask();
    }

    ///@TODO UVR
//    if (m_framework->GetGuiController()->OnTapMoved(pt))
//      return;

    if (m_isDrag)
      m_framework->DoDrag(GetDragEvent(e));

    if (m_isRotate)
      m_framework->DoRotate(GetRotateEvent(e->pos()));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    TBase::mouseReleaseEvent(e);

    m2::PointD const pt = GetDevicePoint(e);
    ///@TODO UVR
//    if (m_framework->GetGuiController()->OnTapEnded(pt))
//      return;

    if (!m_wasLongClick && m_isCleanSingleClick)
    {
      m_framework->GetBalloonManager().RemovePin();

      StartPressTask(pt, SHORT_TOUCH_MS);
    }
    else
      m_wasLongClick = false;

    StopDragging(e);
    StopRotating(e);
  }

  void DrawWidget::keyReleaseEvent(QKeyEvent * e)
  {
    TBase::keyReleaseEvent(e);

    StopRotating(e);

    if (e->key() == Qt::Key_Alt)
      m_emulatingLocation = false;
  }

  void DrawWidget::sizeChanged(int)
  {
    double scaleFactor = static_cast<double>(devicePixelRatio());
    m_framework->OnSize(scaleFactor * width(), scaleFactor * height());

    UpdateScaleControl();
  }

  void DrawWidget::StopRotating(QMouseEvent * e)
  {
    if (m_isRotate && (e->button() == Qt::LeftButton))
    {
      m_framework->StopRotate(GetRotateEvent(e->pos()));

      setCursor(Qt::ArrowCursor);
      m_isRotate = false;
    }
  }

  void DrawWidget::StopRotating(QKeyEvent * e)
  {
    if (m_isRotate && (e->key() == Qt::Key_Control))
      m_framework->StopRotate(GetRotateEvent(mapFromGlobal(QCursor::pos())));
  }

  void DrawWidget::StopDragging(QMouseEvent * e)
  {
    if (m_isDrag && e->button() == Qt::LeftButton)
    {
      m_framework->StopDrag(GetDragEvent(e));

      setCursor(Qt::ArrowCursor);
      m_isDrag = false;
    }
  }

  void DrawWidget::wheelEvent(QWheelEvent * e)
  {
    if (!m_isDrag && !m_isRotate)
    {
      m_framework->ScaleToPoint(ScaleToPointEvent(L2D(e->x()), L2D(e->y()), exp(e->delta() / 360.0)));

      UpdateScaleControl();
    }
  }

  void DrawWidget::UpdateScaleControl()
  {
    if (m_pScale && isExposed())
    {
      // don't send ScaleChanged
      m_pScale->SetPosWithBlockedSignals(m_framework->GetDrawScale());
    }
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

    UpdateScaleControl();
  }

  void DrawWidget::CloseSearch()
  {
    ///@TODO UVR
    //setFocus();
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

  void DrawWidget::SetRouter(routing::RouterType routerType)
  {
    m_framework->SetRouter(routerType);
  }
}
