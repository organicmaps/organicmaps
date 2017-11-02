#include "qt/create_feature_dialog.hpp"
#include "qt/draw_widget.hpp"
#include "qt/editor_dialog.hpp"
#include "qt/place_page_dialog.hpp"
#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "search/result.hpp"

#include "storage/index.hpp"

#include "indexer/editable_map_object.hpp"

#include "platform/settings.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QGuiApplication>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QMenu>

#include <QtCore/QLocale>
#include <QtCore/QThread>
#include <QtCore/QTimer>

using namespace qt::common;

namespace qt
{
DrawWidget::DrawWidget(Framework & framework, bool apiOpenGLES3, QWidget * parent)
  : TBase(framework, apiOpenGLES3, parent)
  , m_rubberBand(nullptr)
  , m_emulatingLocation(false)
{
  m_framework.SetMapSelectionListeners(
      [this](place_page::Info const & info) { ShowPlacePage(info); },
      [](bool /* switchFullScreenMode */) {});  // Empty deactivation listener.

  m_framework.GetRoutingManager().SetRouteBuildingListener(
      [](routing::IRouter::ResultCode, storage::TCountriesVec const &) {});

  m_framework.GetRoutingManager().SetRouteRecommendationListener(
    [this](RoutingManager::Recommendation r)
  {
    OnRouteRecommendation(r);
  });

  m_framework.SetCurrentCountryChangedListener([this](storage::TCountryId const & countryId) {
    m_countryId = countryId;
    UpdateCountryStatus(countryId);
  });

  QTimer * countryStatusTimer = new QTimer(this);
  VERIFY(connect(countryStatusTimer, SIGNAL(timeout()), this, SLOT(OnUpdateCountryStatusByTimer())), ());
  countryStatusTimer->setSingleShot(false);
  countryStatusTimer->start(1000);
}

DrawWidget::~DrawWidget()
{
  delete m_rubberBand;
}

void DrawWidget::UpdateCountryStatus(storage::TCountryId const & countryId)
{
  if (m_currentCountryChanged != nullptr)
  {
    string countryName = countryId;
    auto status = m_framework.GetStorage().CountryStatusEx(countryId);

    uint8_t progressInPercentage = 0;
    storage::MapFilesDownloader::TProgress progressInByte = make_pair(0, 0);
    if (!countryId.empty())
    {
      storage::NodeAttrs nodeAttrs;
      m_framework.GetStorage().GetNodeAttrs(countryId, nodeAttrs);
      progressInByte = nodeAttrs.m_downloadingProgress;
      if (progressInByte.second != 0)
        progressInPercentage = static_cast<int8_t>(100 * progressInByte.first / progressInByte.second);
    }

    m_currentCountryChanged(countryId, countryName, status,
                            progressInByte.second, progressInPercentage);
  }
}

void DrawWidget::OnUpdateCountryStatusByTimer()
{
  if (!m_countryId.empty())
    UpdateCountryStatus(m_countryId);
}

void DrawWidget::SetCurrentCountryChangedListener(DrawWidget::TCurrentCountryChanged const & listener)
{
  m_currentCountryChanged = listener;
}

void DrawWidget::DownloadCountry(storage::TCountryId const & countryId)
{
  m_framework.GetStorage().DownloadNode(countryId);
  if (!m_countryId.empty())
    UpdateCountryStatus(m_countryId);
}

void DrawWidget::RetryToDownloadCountry(storage::TCountryId const & countryId)
{
  // TODO @bykoianko
}

void DrawWidget::PrepareShutdown()
{
  auto & routingManager = m_framework.GetRoutingManager();
  if (routingManager.IsRoutingActive() && routingManager.IsRoutingFollowing())
  {
    routingManager.SaveRoutePoints();

    auto style = m_framework.GetMapStyle();
    if (style == MapStyle::MapStyleVehicleClear)
      m_framework.MarkMapStyle(MapStyle::MapStyleClear);
    else if (style == MapStyle::MapStyleVehicleDark)
      m_framework.MarkMapStyle(MapStyle::MapStyleDark);
  }
}

void DrawWidget::UpdateAfterSettingsChanged()
{
  m_framework.EnterForeground();
}

void DrawWidget::ShowAll()
{
  m_framework.ShowAll();
}

void DrawWidget::ChoosePositionModeEnable()
{
  m_framework.BlockTapEvents(true /* block */);
  m_framework.EnableChoosePositionMode(true /* enable */, false /* enableBounds */,
                                       false /* applyPosition */, m2::PointD() /* position */);
}

void DrawWidget::ChoosePositionModeDisable()
{
  m_framework.EnableChoosePositionMode(false /* enable */, false /* enableBounds */,
                                       false /* applyPosition */, m2::PointD() /* position */);
  m_framework.BlockTapEvents(false /* block */);
}

void DrawWidget::initializeGL()
{
  MapWidget::initializeGL();
  m_framework.LoadBookmarks();

  auto & routingManager = m_framework.GetRoutingManager();
  if (routingManager.LoadRoutePoints())
    routingManager.BuildRoute(0 /* timeoutSec */);
}

void DrawWidget::mousePressEvent(QMouseEvent * e)
{
  QOpenGLWidget::mousePressEvent(e);

  m2::PointD const pt = GetDevicePoint(e);

  if (IsLeftButton(e))
  {
    if (IsShiftModifier(e))
      SubmitRoutingPoint(pt);
    else if (IsAltModifier(e))
      SubmitFakeLocationPoint(pt);
    else
      m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_DOWN));
  }
  else if (IsRightButton(e))
  {
    if (IsAltModifier(e))
    {
      SubmitBookmark(pt);
    }
    else if (!m_selectionMode || IsCommandModifier(e))
    {
      ShowInfoPopup(e, pt);
    }
    else
    {
      m_rubberBandOrigin = e->pos();
      if (m_rubberBand == nullptr)
        m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
      m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
      m_rubberBand->show();
    }
  }
}

void DrawWidget::mouseMoveEvent(QMouseEvent * e)
{
  QOpenGLWidget::mouseMoveEvent(e);
  if (IsLeftButton(e) && !IsAltModifier(e))
    m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_MOVE));

  if (m_selectionMode && m_rubberBand != nullptr && m_rubberBand->isVisible())
  {
    m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, e->pos()).normalized());
  }
}

void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
{
  QOpenGLWidget::mouseReleaseEvent(e);
  if (IsLeftButton(e) && !IsAltModifier(e))
  {
    m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_UP));
  }
  else if (m_selectionMode && IsRightButton(e) && m_rubberBand != nullptr &&
           m_rubberBand->isVisible())
  {
    QPoint const lt = m_rubberBand->geometry().topLeft();
    QPoint const rb = m_rubberBand->geometry().bottomRight();
    m2::RectD rect;
    rect.Add(m_framework.PtoG(m2::PointD(L2D(lt.x()), L2D(lt.y()))));
    rect.Add(m_framework.PtoG(m2::PointD(L2D(rb.x()), L2D(rb.y()))));
    m_framework.VisualizeRoadsInRect(rect);
    m_rubberBand->hide();
  }
}

void DrawWidget::keyPressEvent(QKeyEvent * e)
{
  TBase::keyPressEvent(e);
  if (IsLeftButton(QGuiApplication::mouseButtons()) &&
      e->key() == Qt::Key_Control)
  {
    df::TouchEvent event;
    event.SetTouchType(df::TouchEvent::TOUCH_DOWN);
    df::Touch touch;
    touch.m_id = 0;
    touch.m_location = m2::PointD(L2D(QCursor::pos().x()), L2D(QCursor::pos().y()));
    event.SetFirstTouch(touch);
    event.SetSecondTouch(GetSymmetrical(touch));

    m_framework.TouchEvent(event);
  }
}

void DrawWidget::keyReleaseEvent(QKeyEvent * e)
{
  TBase::keyReleaseEvent(e);

  if (IsLeftButton(QGuiApplication::mouseButtons()) &&
      e->key() == Qt::Key_Control)
  {
    df::TouchEvent event;
    event.SetTouchType(df::TouchEvent::TOUCH_UP);
    df::Touch touch;
    touch.m_id = 0;
    touch.m_location = m2::PointD(L2D(QCursor::pos().x()), L2D(QCursor::pos().y()));
    event.SetFirstTouch(touch);
    event.SetSecondTouch(GetSymmetrical(touch));

    m_framework.TouchEvent(event);
  }
  else if (e->key() == Qt::Key_Alt)
    m_emulatingLocation = false;
}

bool DrawWidget::Search(search::EverywhereSearchParams const & params)
{
  return m_framework.SearchEverywhere(params);
}

string DrawWidget::GetDistance(search::Result const & res) const
{
  string dist;
  if (auto const position = m_framework.GetCurrentPosition())
  {
    auto const ll = MercatorBounds::ToLatLon(*position);
    double dummy;
    (void)m_framework.GetDistanceAndAzimut(res.GetFeatureCenter(), ll.lat, ll.lon, -1.0, dist,
                                           dummy);
  }
  return dist;
}

void DrawWidget::ShowSearchResult(search::Result const & res)
{
  m_framework.ShowSearchResult(res);
}

void DrawWidget::CreateFeature()
{
  auto cats = m_framework.GetEditorCategories();
  CreateFeatureDialog dlg(this, cats);
  if (dlg.exec() == QDialog::Accepted)
  {
    osm::EditableMapObject emo;
    if (m_framework.CreateMapObject(m_framework.GetViewportCenter(), dlg.GetSelectedType(), emo))
    {
      EditorDialog dlg(this, emo);
      int const result = dlg.exec();
      if (result == QDialog::Accepted)
        m_framework.SaveEditedMapObject(emo);
    }
    else
    {
      LOG(LWARNING, ("Error creating new map object."));
    }
  }
}

void DrawWidget::OnLocationUpdate(location::GpsInfo const & info)
{
  if (!m_emulatingLocation)
    m_framework.OnLocationUpdate(info);
}

void DrawWidget::SetMapStyle(MapStyle mapStyle)
{
  m_framework.SetMapStyle(mapStyle);
}

void DrawWidget::SubmitFakeLocationPoint(m2::PointD const & pt)
{
  m_emulatingLocation = true;
  auto const point = m_framework.P3dtoG(pt);
  m_framework.OnLocationUpdate(qt::common::MakeGpsInfo(point));

  if (m_framework.GetRoutingManager().IsRoutingActive())
  {
    location::FollowingInfo loc;
    m_framework.GetRoutingManager().GetRouteFollowingInfo(loc);
    LOG(LDEBUG, ("Distance:", loc.m_distToTarget, loc.m_targetUnitsSuffix, "Time:", loc.m_time,
                 "Turn:", routing::turns::GetTurnString(loc.m_turn), "(", loc.m_distToTurn, loc.m_turnUnitsSuffix,
                 ") Roundabout exit number:", loc.m_exitNum));
  }
}

void DrawWidget::SubmitRoutingPoint(m2::PointD const & pt)
{
  auto & routingManager = m_framework.GetRoutingManager();
  auto const pointsCount = routingManager.GetRoutePoints().size();

  // Check if limit of intermediate points is reached.
  if (m_routePointAddMode == RouteMarkType::Intermediate && !routingManager.CouldAddIntermediatePoint())
    routingManager.RemoveRoutePoint(RouteMarkType::Intermediate, 0);

  // Insert implicit start point.
  if (m_routePointAddMode == RouteMarkType::Finish && pointsCount == 0)
  {
    RouteMarkData startPoint;
    startPoint.m_pointType = RouteMarkType::Start;
    startPoint.m_isMyPosition = true;
    routingManager.AddRoutePoint(std::move(startPoint));
  }

  RouteMarkData point;
  point.m_pointType = m_routePointAddMode;
  point.m_isMyPosition = false;
  point.m_position = m_framework.P3dtoG(pt);
  routingManager.AddRoutePoint(std::move(point));

  if (routingManager.GetRoutePoints().size() >= 2)
    routingManager.BuildRoute(0 /* timeoutSec */);
}

void DrawWidget::SubmitBookmark(m2::PointD const & pt)
{
  size_t categoryIndex = 0;
  auto category = m_framework.GetBookmarkManager().GetBmCategory(categoryIndex);
  if (category == nullptr)
    categoryIndex = m_framework.GetBookmarkManager().CreateBmCategory("Desktop_bookmarks");
  BookmarkData data("", "placemark-red");
  m_framework.GetBookmarkManager().AddBookmark(categoryIndex, m_framework.P3dtoG(pt), data);
}

void DrawWidget::FollowRoute()
{
  auto & routingManager = m_framework.GetRoutingManager();

  auto const points = routingManager.GetRoutePoints();
  if (points.size() < 2)
    return;
  if (!points.front().m_isMyPosition && !points.back().m_isMyPosition)
    return;
  if (routingManager.IsRoutingActive() && !routingManager.IsRoutingFollowing())
  {
    routingManager.FollowRoute();
    auto style = m_framework.GetMapStyle();
    if (style == MapStyle::MapStyleClear)
      SetMapStyle(MapStyle::MapStyleVehicleClear);
    else if (style == MapStyle::MapStyleDark)
      SetMapStyle(MapStyle::MapStyleVehicleDark);
  }
}

void DrawWidget::ClearRoute()
{
  auto & routingManager = m_framework.GetRoutingManager();

  bool const wasActive = routingManager.IsRoutingActive() && routingManager.IsRoutingFollowing();
  routingManager.CloseRouting(true /* remove route points */);

  if (wasActive)
  {
    auto style = m_framework.GetMapStyle();
    if (style == MapStyle::MapStyleVehicleClear)
      SetMapStyle(MapStyle::MapStyleClear);
    else if (style == MapStyle::MapStyleVehicleDark)
      SetMapStyle(MapStyle::MapStyleDark);
  }
}

void DrawWidget::OnRouteRecommendation(RoutingManager::Recommendation recommendation)
{
  if (recommendation == RoutingManager::Recommendation::RebuildAfterPointsLoading)
  {
    auto & routingManager = m_framework.GetRoutingManager();

    RouteMarkData startPoint;
    startPoint.m_pointType = RouteMarkType::Start;
    startPoint.m_isMyPosition = true;
    routingManager.AddRoutePoint(std::move(startPoint));

    if (routingManager.GetRoutePoints().size() >= 2)
      routingManager.BuildRoute(0 /* timeoutSec */);
  }
}

void DrawWidget::ShowPlacePage(place_page::Info const & info)
{
  search::AddressInfo address;
  if (info.IsFeature())
    address = m_framework.GetFeatureAddressInfo(info.GetID());
  else
    address = m_framework.GetAddressInfoAtPoint(info.GetMercator());

  PlacePageDialog dlg(this, info, address);
  if (dlg.exec() == QDialog::Accepted)
  {
    osm::EditableMapObject emo;
    if (m_framework.GetEditableMapObject(info.GetID(), emo))
    {
      EditorDialog dlg(this, emo);
      int const result = dlg.exec();
      if (result == QDialog::Accepted)
      {
        m_framework.SaveEditedMapObject(emo);
        m_framework.UpdatePlacePageInfoForCurrentSelection();
      }
      else if (result == QDialogButtonBox::DestructiveRole)
      {
        m_framework.DeleteFeature(info.GetID());
      }
    }
    else
    {
      LOG(LERROR, ("Error while trying to edit feature."));
    }
  }
  m_framework.DeactivateMapSelection(false);
}

void DrawWidget::ShowInfoPopup(QMouseEvent * e, m2::PointD const & pt)
{
  // show feature types
  QMenu menu;
  auto const addStringFn = [&menu](string const & s)
  {
    menu.addAction(QString::fromUtf8(s.c_str()));
  };

  m_framework.ForEachFeatureAtPoint([&](FeatureType & ft)
  {
    search::AddressInfo const info = m_framework.GetFeatureAddressInfo(ft);

    string concat;
    for (auto const & type : info.m_types)
      concat += type + " ";
    addStringFn(concat);

    if (!info.m_name.empty())
      addStringFn(info.m_name);

    string const addr = info.FormatAddress();
    if (!addr.empty())
      addStringFn(addr);

    menu.addSeparator();
  }, m_framework.PtoG(pt));

  menu.exec(e->pos());
}

void DrawWidget::SetRouter(routing::RouterType routerType)
{
  m_framework.GetRoutingManager().SetRouter(routerType);
}

void DrawWidget::SetSelectionMode(bool mode) { m_selectionMode = mode; }

// static
void DrawWidget::SetDefaultSurfaceFormat(bool apiOpenGLES3)
{
  QSurfaceFormat fmt;
  fmt.setAlphaBufferSize(8);
  fmt.setBlueBufferSize(8);
  fmt.setGreenBufferSize(8);
  fmt.setRedBufferSize(8);
  fmt.setStencilBufferSize(0);
  fmt.setSamples(0);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setSwapInterval(1);
  fmt.setDepthBufferSize(16);
  if (apiOpenGLES3)
  {
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 2);
  }
  else
  {
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 1);
  }
  //fmt.setOption(QSurfaceFormat::DebugContext);
  QSurfaceFormat::setDefaultFormat(fmt);
}

void DrawWidget::RefreshDrawingRules()
{
  SetMapStyle(MapStyleClear);
}
}  // namespace qt
