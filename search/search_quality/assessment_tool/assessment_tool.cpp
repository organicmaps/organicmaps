#include "search/search_quality/assessment_tool/main_model.hpp"
#include "search/search_quality/assessment_tool/main_view.hpp"

#include "map/framework.hpp"

#include "search/search_quality/helpers.hpp"

#include "platform/platform_tests_support/helpers.hpp"

#include "platform/platform.hpp"

#include <QtGui/QScreen>
#include <QtWidgets/QApplication>

#include <gflags/gflags.h>

DEFINE_string(resources_path, "", "Path to resources directory");
DEFINE_string(data_path, "", "Path to data directory");
DEFINE_string(samples_path, "", "Path to the file with samples to open on startup");
DEFINE_uint64(num_threads, 4, "Number of search engine threads");

int main(int argc, char ** argv)
{
  platform::tests_support::ChangeMaxNumberOfOpenFiles(search::search_quality::kMaxOpenFiles);

  gflags::SetUsageMessage("Assessment tool.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();
  if (!FLAGS_resources_path.empty())
    platform.SetResourceDir(FLAGS_resources_path);
  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  Q_INIT_RESOURCE(resources_common);
  QApplication app(argc, argv);
  // Pretty icons on HDPI displays.
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  FrameworkParams params;
  CHECK_GREATER(FLAGS_num_threads, 0, ());
  params.m_numSearchAPIThreads = FLAGS_num_threads;

  Framework framework(params);
  MainView view(framework, app.primaryScreen()->geometry());
  MainModel model(framework);

  model.SetView(view);
  view.SetModel(model);

  view.showMaximized();

  if (!FLAGS_samples_path.empty())
    model.Open(FLAGS_samples_path);

  return app.exec();
}
