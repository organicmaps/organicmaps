#include "testing/testing.hpp"

#include "qt/qt_common/map_widget_container.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

namespace map_widget_container_tests
{
namespace
{
class Application
{
  int m_argc = 1;
  char m_name[sizeof("map_widget_container_tests")] = "map_widget_container_tests";
  char * m_argv[2] = {m_name, nullptr};
  QApplication m_app{m_argc, m_argv};
};

void ProcessLayout()
{
  // A layout request can post the container's update, which can post another layout request.
  for (int i = 0; i < 4; ++i)
    QApplication::processEvents();
}

class Map : public QWidget
{
public:
  using QWidget::QWidget;
  int m_resizes = 0;

protected:
  void resizeEvent(QResizeEvent *) override { ++m_resizes; }
};

class Window : public QMainWindow
{
public:
  Window()
  {
    resize(1000, 700);
    setAnimated(false);
    addToolBar("Toolbar")->addAction("Action");
    statusBar()->showMessage("Status");
    m_map = new Map(this);
    m_container = new qt::common::MapWidgetContainer(*this, *m_map);
    setCentralWidget(m_container);
    connect(m_container, &qt::common::MapWidgetContainer::VisibleViewportChanged, this,
            [this](QRect const & viewport) { m_viewport = viewport; });
    m_left = AddDock(Qt::LeftDockWidgetArea);
    m_right = AddDock(Qt::RightDockWidgetArea);
    m_top = AddDock(Qt::TopDockWidgetArea);
  }

  QDockWidget * AddDock(Qt::DockWidgetArea area)
  {
    auto * dock = new QDockWidget("Panel", this);
    dock->setWidget(new QLabel("Contents"));
    addDockWidget(area, dock);
    dock->hide();
    return dock;
  }

  Map * m_map;
  qt::common::MapWidgetContainer * m_container;
  QDockWidget * m_left;
  QDockWidget * m_right;
  QDockWidget * m_top;
  QRect m_viewport;
};

void ProcessEvents(df::UserEventStream & stream)
{
  bool model, viewport, active;
  stream.ProcessEvents(model, viewport, active);
}

void ResetCamera(df::UserEventStream & stream)
{
  stream.AddEvent(make_unique_dp<df::SetRectEvent>(m2::RectD(-1, -0.6, 1, 0.6), false, -1, false, false, nullptr));
  ProcessEvents(stream);
}
}  // namespace

UNIT_TEST(MapContainer_DocksKeepSurfaceAndInputCoordinates)
{
  Application app;
  Window window;
  window.show();
  ProcessLayout();
  auto const size = window.m_map->size();
  auto const origin = window.m_map->mapTo(&window, QPoint());
  auto const resizes = window.m_map->m_resizes;
  QPoint const landmark(500, 300);
  auto const position = window.m_map->mapTo(&window, landmark);

  auto const check = [&]
  {
    ProcessLayout();
    TEST(window.m_map->size() == size, ());
    TEST(window.m_map->mapTo(&window, QPoint()) == origin, ());
    TEST_EQUAL(window.m_map->m_resizes, resizes, ());
    TEST(window.m_map->mapTo(&window, landmark) == position, ());
    TEST(window.m_viewport == QRect(window.m_map->mapFrom(window.m_container, QPoint()), window.m_container->size()),
         ());
    TEST(window.m_viewport.contains(landmark), ());
    QPoint const input = window.m_map->mapTo(window.m_container, landmark);
    TEST(window.m_container->childAt(input) == window.m_map, ());
    TEST(window.m_map->mapFrom(window.m_container, input) == landmark, ());
  };

  for (int i = 0; i < 3; ++i)
  {
    window.m_left->show();
    check();
    window.resizeDocks({window.m_left}, {250}, Qt::Horizontal);
    check();
    window.m_right->show();
    check();
    window.resizeDocks({window.m_right}, {200}, Qt::Horizontal);
    check();
    window.m_top->show();
    check();
    window.m_top->hide();
    window.m_left->hide();
    check();
    window.m_right->hide();
    check();
  }
}

UNIT_TEST(MapContainer_WindowResizeAndFloatingDocks)
{
  Application app;
  Window window;
  window.m_left->show();
  window.show();
  ProcessLayout();
  auto const size = window.m_map->size();
  auto const origin = window.m_map->mapTo(&window, QPoint());
  TEST_GREATER(window.m_viewport.x(), 0, ());

  window.resize(window.width() + 200, window.height() + 100);
  ProcessLayout();
  TEST(window.m_map->size() == size + QSize(200, 100), ());
  TEST(window.m_map->mapTo(&window, QPoint()) == origin, ());

  auto const resized = window.m_map->size();
  auto const resizes = window.m_map->m_resizes;
  window.m_left->setFloating(true);
  ProcessLayout();
  TEST(window.m_map->size() == resized, ());
  TEST_EQUAL(window.m_map->m_resizes, resizes, ());
  TEST_EQUAL(window.m_viewport.x(), 0, ());

  window.m_left->setFloating(false);
  window.addDockWidget(Qt::RightDockWidgetArea, window.m_left);
  ProcessLayout();
  TEST(window.m_map->size() == resized, ());
  TEST(window.m_map->mapTo(&window, QPoint()) == origin, ());
  TEST_LESS(window.m_viewport.width(), resized.width(), ());
  TEST_EQUAL(window.m_viewport.x(), 0, ());
}

UNIT_TEST(MapContainer_OverlayControlsStayInVisibleArea)
{
  Application app;
  Window window;
  auto * layout = new QHBoxLayout(window.m_container);
  auto * button = new QPushButton("Download");
  layout->addWidget(button, 0, Qt::AlignHCenter);
  window.show();
  ProcessLayout();
  window.m_left->show();
  window.resizeDocks({window.m_left}, {300}, Qt::Horizontal);
  ProcessLayout();
  TEST_ALMOST_EQUAL_ABS(button->geometry().center().x(), window.m_container->rect().center().x(), 1, ());
  TEST(window.m_viewport.contains(window.m_map->mapFrom(window.m_container, button->geometry().center())), ());
}

UNIT_TEST(MapContainer_DockMoveWithoutVisibleAreaResize)
{
  Application app;
  Window window;
  window.m_left->show();
  window.show();
  window.resizeDocks({window.m_left}, {300}, Qt::Horizontal);
  ProcessLayout();
  auto const visibleSize = window.m_container->size();
  auto const visibleOrigin = window.m_container->pos();
  auto const mapSize = window.m_map->size();
  auto const mapOrigin = window.m_map->mapTo(&window, QPoint());
  auto const resizes = window.m_map->m_resizes;
  TEST_GREATER(window.m_viewport.x(), 0, ());

  window.addDockWidget(Qt::RightDockWidgetArea, window.m_left);
  window.resizeDocks({window.m_left}, {300}, Qt::Horizontal);
  ProcessLayout();
  TEST(window.m_container->size() == visibleSize, ());
  TEST(window.m_container->pos() != visibleOrigin, ());
  TEST_EQUAL(window.m_viewport.x(), 0, ());
  TEST(window.m_map->size() == mapSize, ());
  TEST(window.m_map->mapTo(&window, QPoint()) == mapOrigin, ());
  TEST_EQUAL(window.m_map->m_resizes, resizes, ());
}

using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, MapContainer_VisibleAreaKeepsCameraAndPendingSearchZoom)
{
  Application app;
  Window window;
  window.show();
  ProcessLayout();
  Qt::DockWidgetAreas const layouts[] = {Qt::LeftDockWidgetArea, Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
                                         Qt::RightDockWidgetArea, Qt::TopDockWidgetArea | Qt::RightDockWidgetArea,
                                         Qt::NoDockWidgetArea};
  for (double ratio : {1.0, 1.5, 2.0})
  {
    for (double angle : {0.0, 0.7})
    {
      for (bool perspective : {false, true})
      {
        df::UserEventStream stream;
        stream.AddEvent(make_unique_dp<df::ResizeEvent>(window.m_map->width() * ratio, window.m_map->height() * ratio));
        ResetCamera(stream);
        stream.AddEvent(make_unique_dp<df::SetAnyRectEvent>(
            m2::AnyRectD(m2::PointD(10, 20), ang::AngleD(angle), m2::RectD(-0.001, -0.0006, 0.001, 0.0006)), false,
            false, false));
        stream.AddEvent(make_unique_dp<df::SetAutoPerspectiveEvent>(perspective));
        ProcessEvents(stream);
        auto const initial = stream.GetCurrentScreen();
        TEST_EQUAL(initial.isPerspective(), perspective, ());
        m2::PointD const pixel(window.m_map->width() * 0.75 * ratio, window.m_map->height() * 0.6 * ratio);
        auto const target = initial.PtoG(initial.P3dtoP(pixel));
        auto const projection = initial.PtoP3d(initial.GtoP(target));
        QObject observer;
        QObject::connect(window.m_container, &qt::common::MapWidgetContainer::VisibleViewportChanged, &observer,
                         [&stream, ratio](QRect const & viewport)
        {
          stream.AddEvent(make_unique_dp<df::SetVisibleViewportEvent>(
              m2::RectD(viewport.x() * ratio, viewport.y() * ratio, (viewport.x() + viewport.width()) * ratio,
                        (viewport.y() + viewport.height()) * ratio)));
        });
        for (bool trackSearch : {false, true})
        {
          if (trackSearch)
          {
            stream.AddEvent(make_unique_dp<df::SetCenterEvent>(target, 14, true, true, nullptr));
            ProcessEvents(stream);
          }
          ScreenBase expected;
          stream.GetTargetScreen(expected);
          for (auto const areas : layouts)
          {
            window.m_left->setVisible(areas.testFlag(Qt::LeftDockWidgetArea));
            window.m_right->setVisible(areas.testFlag(Qt::RightDockWidgetArea));
            window.m_top->setVisible(areas.testFlag(Qt::TopDockWidgetArea));
            ProcessLayout();
            ProcessEvents(stream);
            ScreenBase screen;
            stream.GetTargetScreen(screen);
            TEST_EQUAL(screen.PixelRectIn3d(), initial.PixelRectIn3d(), ());
            QPointF const center = QRectF(window.m_viewport).center() * ratio;
            auto const expectedPosition = trackSearch ? m2::PointD(center.x(), center.y()) : projection;
            TEST_ALMOST_EQUAL_ABS(screen.PtoP3d(screen.GtoP(target)).Length(expectedPosition), 0.0, 1e-3, ());
            TEST_ALMOST_EQUAL_ABS(screen.GetScale(), expected.GetScale(), 1e-12, ());
          }
        }
        ResetCamera(stream);
      }
    }
  }
}
}  // namespace map_widget_container_tests
