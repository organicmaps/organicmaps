#include "search/search_quality/assessment_tool/main_model.hpp"
#include "search/search_quality/assessment_tool/main_view.hpp"

#include "search/search_quality/helpers.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"

#include <QtWidgets/QApplication>

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(resources_path, "", "Path to resources directory");
DEFINE_string(data_path, "", "Path to data directory");

namespace
{
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
  MainView view(framework);
  MainModel model;

  model.SetView(view);
  view.SetModel(model);

  view.show();
  return app.exec();
}
