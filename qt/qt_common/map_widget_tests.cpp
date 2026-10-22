#include "testing/testing.hpp"

#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/map_widget_container.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/search_api.hpp"

#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"

#include <QtCore/QDeadlineTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

#include <cmath>
#include <utility>

namespace
{
class TestFramework final : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}
  ScreenBase const & GetScreen() const { return m_rendererScreen; }

  void ObserveRendererScreen()
  {
    auto engine = GetDrapeEngine();
    engine->SetRenderingDisabled(false /* destroySurface */);
    m_rendererScreen = m_currentModelView;
    // Camera assertions need every update, including those filtered out by Framework.
    engine->SetModelViewListener([this](ScreenBase const & screen)
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [this, screen]
      {
        m_rendererScreen = screen;
        OnViewportChanged(screen);
      });
    });
    engine->SetRenderingEnabled();
  }

private:
  ScreenBase m_rendererScreen;
};

class MapSurface : public QOpenGLWidget
{
public:
  using QOpenGLWidget::QOpenGLWidget;
  int m_resizes = 0;
  QPoint const m_landmark{700, 300};

protected:
  void resizeGL(int, int) override { ++m_resizes; }
  void paintGL() override
  {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::blue);
    painter.fillRect(QRect(m_landmark - QPoint(4, 4), QSize(9, 9)), Qt::red);
  }
};
template <typename Predicate>
bool WaitUntil(Predicate const & ready)
{
  QDeadlineTimer deadline(5000);
  do
  {
    QApplication::processEvents();
    if (ready())
      return true;
    QThread::msleep(1);
  }
  while (!deadline.hasExpired());
  return false;
}

}  // namespace

UNIT_TEST(QtMap_DocksKeepSurfaceAndRefreshViewportSearch)
{
  Q_INIT_RESOURCE(resources_common);
  RunTestInOpenGLOffscreenEnvironment("Map dock integration", []
  {
    qt::common::SetDefaultSurfaceFormat(QApplication::platformName());
    int searchesCompleted = 0;
    TestFramework framework;
    auto const & screen = framework.GetScreen();
    framework.AddString("core_placepage_unknown_place", "Unknown place");
    TEST(!framework.GetSearchAPI().IsViewportInitialized(), ());

    QMainWindow window;
    window.resize(1000, 700);
    window.setAnimated(false);
    auto * map = new qt::common::MapWidget(framework, false /* isScreenshotMode */, &window);
    auto * container = new qt::common::MapWidgetContainer(window, *map);
    window.setCentralWidget(container);
    QObject::connect(container, &qt::common::MapWidgetContainer::VisibleViewportChanged, map,
                     &qt::common::MapWidget::SetVisibleViewport);
    auto * dock = new QDockWidget("Place page", &window);
    dock->setWidget(new QLabel("Selected place"));
    dock->setMinimumWidth(300);
    window.addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->hide();
    framework.SetPlacePageListeners([&] { dock->show(); }, [&] { dock->hide(); }, {}, {});
    window.show();
    TEST(WaitUntil([&] { return map->isValid() && framework.GetSearchAPI().IsViewportInitialized(); }), ());

    framework.ObserveRendererScreen();
    framework.SetViewportCenter(m2::PointD(10, 20), 14, false /* isAnim */);
    TEST(WaitUntil(
             [&]
    {
      return screen.GetOrg().Length(m2::PointD(10, 20)) < 1e-8 &&
             std::abs(screen.GetScale() - df::GetScreenScale(14)) < 1e-12;
    }),
         ());
    auto const initial = screen;
    auto const surface = framework.GetCurrentViewport();
    auto const mapSize = map->size();
    double const ratio = map->devicePixelRatioF();

    search::ViewportSearchParams params;
    params.m_query = "cafe ";
    params.m_inputLocale = "en";
    params.m_onCompleted = [&](search::Results const & results)
    {
      if (results.IsEndedNormal())
        ++searchesCompleted;
    };
    framework.GetSearchAPI().SearchInViewport(std::move(params));
    TEST(WaitUntil([&] { return searchesCompleted > 0; }), ());
    auto const beforeDock = searchesCompleted;
    dock->show();
    TEST(WaitUntil([&] { return searchesCompleted > beforeDock; }), ("Dock did not refresh viewport search"));
    TEST(map->size() == mapSize, ());
    TEST_EQUAL(framework.GetCurrentViewport(), surface, ());
    auto const covered = initial.PtoG(initial.P3dtoP(m2::PointD(100 * ratio, 350 * ratio)));
    TEST(!framework.GetVisibleViewport().IsPointInside(covered), ());

    auto const beforeResize = searchesCompleted;
    window.resizeDocks({dock}, {450}, Qt::Horizontal);
    TEST(WaitUntil([&] { return searchesCompleted > beforeResize; }), ("Dock resize did not refresh search"));
    TEST_EQUAL(framework.GetCurrentViewport(), surface, ());
    auto const beforeClose = searchesCompleted;
    dock->hide();
    TEST(WaitUntil([&] { return searchesCompleted > beforeClose; }), ("Closing dock did not refresh search"));
    TEST_EQUAL(framework.GetVisibleViewport(), surface, ());
    framework.GetSearchAPI().CancelSearch(search::Mode::Viewport);

    m2::PointD const visiblePixel(750 * ratio, 350 * ratio);
    auto const visiblePoint = initial.PtoG(initial.P3dtoP(visiblePixel));
    place_page::BuildInfo info;
    info.m_source = place_page::BuildInfo::Source::User;
    info.m_match = place_page::BuildInfo::Match::Nothing;
    info.m_mercator = covered;
    framework.BuildAndSetPlacePageInfo(info);
    TEST(WaitUntil(
             [&]
    {
      auto const pixel = screen.PtoP3d(screen.GtoP(covered));
      auto const origin = map->mapFrom(container, QPoint());
      return dock->isVisible() && origin.x() > 0 && pixel.x > origin.x() * ratio &&
             pixel.x < (origin.x() + container->width()) * ratio;
    }),
         ("Covered selection stayed behind the dock"));
    TEST_ALMOST_EQUAL_ABS(screen.GetScale(), initial.GetScale(), 1e-12, ());

    // A new selection can be hidden even when the dock's viewport is unchanged.
    auto const coveredAgain = screen.PtoG(screen.P3dtoP(m2::PointD(100 * ratio, 350 * ratio)));
    info.m_mercator = coveredAgain;
    framework.BuildAndSetPlacePageInfo(info);
    TEST(WaitUntil(
             [&]
    {
      auto const pixel = screen.PtoP3d(screen.GtoP(coveredAgain));
      auto const origin = map->mapFrom(container, QPoint());
      return pixel.x > origin.x() * ratio && pixel.x < (origin.x() + container->width()) * ratio;
    }),
         ("New selection stayed behind the existing dock"));

    framework.SetViewportCenter(visiblePoint, 16, true /* isAnim */, true /* trackVisibleViewport */);
    dock->hide();
    TEST(WaitUntil(
             [&]
    {
      return screen.PtoP3d(screen.GtoP(visiblePoint)).Length(framework.GetVisiblePixelCenter()) < 1e-3 &&
             std::abs(screen.GetScale() - df::GetScreenScale(16)) < 1e-12;
    }),
         ("Dock change lost the search target or zoom"));

    auto const center = screen.GetOrg();
    auto const scale = screen.GetScale();
    framework.SetViewportCenter(center, df::kDoNotChangeZoom, false /* isAnim */);
    // Shrinking also works on CI displays that constrain top-level window height.
    auto const resized = mapSize - QSize(200, 100);
    window.resize(window.width() - 200, window.height() - 100);
    TEST(WaitUntil(
             [&]
    {
      return map->size() == resized && screen.PixelRectIn3d().SizeX() == static_cast<int>(resized.width() * ratio) &&
             screen.PixelRectIn3d().SizeY() == static_cast<int>(resized.height() * ratio);
    }),
         ("Window resize did not reach the map renderer"));
    TEST_ALMOST_EQUAL_ABS(screen.GetOrg().Length(center), 0.0, 1e-8, ());
    TEST_ALMOST_EQUAL_ABS(screen.GetScale(), scale, 1e-12, ());
  });
}

UNIT_TEST(QtMap_DockChangesKeepClippedOpenGLImageStable)
{
  RunTestInOpenGLOffscreenEnvironment("Clipped map surface", []
  {
    QMainWindow window;
    window.resize(1000, 700);
    window.setAnimated(false);
    window.addToolBar("Toolbar")->addAction("Action");
    window.statusBar()->showMessage("Status");
    auto * map = new MapSurface(&window);
    auto * container = new qt::common::MapWidgetContainer(window, *map);
    window.setCentralWidget(container);
    auto * left = new QDockWidget("Place page", &window);
    auto * right = new QDockWidget("Search", &window);
    left->setWidget(new QLabel("Place"));
    right->setWidget(new QLabel("Results"));
    window.addDockWidget(Qt::LeftDockWidgetArea, left);
    window.addDockWidget(Qt::RightDockWidgetArea, right);
    left->hide();
    right->hide();
    window.show();
    auto const processLayout = []
    {
      for (int i = 0; i < 4; ++i)
        QApplication::processEvents();
    };
    processLayout();
    TEST(map->isValid(), ());
    auto const size = map->size();
    auto const resizes = map->m_resizes;
    QPoint const position = map->mapTo(&window, map->m_landmark);
    window.setAnimated(true);
    for (int i = 0; i < 12; ++i)
    {
      left->setVisible(i % 2 == 0);
      right->setVisible(i % 3 == 0);
      window.resizeDocks({left, right}, {300, 200}, Qt::Horizontal);
      processLayout();
      QEventLoop loop;
      QTimer::singleShot(250, &loop, &QEventLoop::quit);
      loop.exec();
      processLayout();
      TEST(map->size() == size, ());
      TEST_EQUAL(map->m_resizes, resizes, ());
      TEST(map->mapTo(&window, map->m_landmark) == position, ());
      // macOS CI's software renderer supports FBOs but cannot composite OpenGL widgets.
      QImage const image = map->grabFramebuffer();
      TEST(!image.isNull(), ());
      TEST(image.pixelColor(map->m_landmark * image.devicePixelRatio()) == QColor(Qt::red), ());
    }
  });
}
