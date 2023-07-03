#include "mainwindow.hpp"

#include "qt/qt_common/helpers.hpp"

#include "map/framework.hpp"

#include <gflags/gflags.h>

#include <QtWidgets/QApplication>

namespace
{
DEFINE_string(resources_path, "", "Path to resources directory");
DEFINE_string(data_path, "", "Path to data directory");
}  // namespace

int main(int argc, char * argv[])
{
  gflags::SetUsageMessage("Visualize and check matched routes.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();
  if (!FLAGS_resources_path.empty())
    platform.SetResourceDir(FLAGS_resources_path);
  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  Q_INIT_RESOURCE(resources_common);
  QApplication app(argc, argv);

  qt::common::SetDefaultSurfaceFormat(app.platformName());

  FrameworkParams params;

  Framework framework(params);
  openlr::MainWindow mainWindow(framework);

  mainWindow.showMaximized();

  return app.exec();
}
