#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "search/search_quality/helpers.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>

#include "3party/gflags/src/gflags/gflags.h"

#include <memory>

DEFINE_string(resources_path, "", "Path to resources directory");
DEFINE_string(data_path, "", "Path to data directory");

namespace
{
class MainWindow : public QMainWindow
{
public:
  MainWindow(Framework & framework) : m_framework(framework)
  {
    setWindowTitle(tr("Assessment tool"));
    InitMenuBar();
    InitMapWidget();
  }

private:
  void InitMenuBar()
  {
    auto * bar = menuBar();

    auto * fileMenu = bar->addMenu(tr("&File"));

    {
      auto open = make_unique<QAction>(tr("&Open queries..."), this);
      open->setShortcuts(QKeySequence::Open);
      open->setStatusTip(tr("Open the file with queries for assessment"));
      connect(open.get(), &QAction::triggered, this, &MainWindow::Open);
      fileMenu->addAction(open.release());
    }

    fileMenu->addSeparator();

    {
      auto quit = make_unique<QAction>(tr("&Quit"), this);
      quit->setShortcuts(QKeySequence::Quit);
      quit->setStatusTip(tr("Exit the tool"));
      connect(quit.get(), &QAction::triggered, this, &QWidget::close);
      fileMenu->addAction(quit.release());
    }
  }

  void InitMapWidget()
  {
    auto widget = make_unique<qt::common::MapWidget>(m_framework, this);
    widget->BindHotkeys(*this);
    InitToolBar(*widget);
    setCentralWidget(widget.release());
  }

  void InitToolBar(qt::common::MapWidget & widget)
  {
    auto toolBar = make_unique<QToolBar>(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setIconSize(QSize(32, 32));
    qt::common::ScaleSlider::Embed(Qt::Vertical, *toolBar, widget);
    addToolBar(Qt::RightToolBarArea, toolBar.release());
  }

  void Open()
  {
    auto const file = QFileDialog::getOpenFileName(this, tr("Open queries..."), QString(),
                                                   tr("JSON files (*.json)"))
                          .toStdString();
    if (file.empty())
      return;
    // TODO (@y): implement this
  }

  Framework & m_framework;
};
}  // namespace

int main(int argc, char ** argv)
{
  search::ChangeMaxNumberOfOpenFiles(search::kMaxOpenFiles);

  google::SetUsageMessage("Features collector tool.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();
  if (!FLAGS_resources_path.empty())
    platform.SetResourceDir(FLAGS_resources_path);
  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  Q_INIT_RESOURCE(resources_common);
  QApplication app(argc, argv);

  Framework framework;
  MainWindow window(framework);
  window.show();
  return app.exec();
}
