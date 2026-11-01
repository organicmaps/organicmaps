#include "testing/testing.hpp"

#include "platform/platform.hpp"

#include <QtCore/QCoreApplication>

#include <glib.h>

#include <atomic>

namespace gui_thread_tests
{
// The app never calls QCoreApplication::exec(): the core's GUI tasks must be run by the GLib main
// loop through Qt's GLib event dispatcher.
UNIT_TEST(GuiThread_TasksRunFromGLibMainLoop)
{
  int argc = 1;
  char arg0[] = "gtk_tests";
  char * argv[] = {arg0, nullptr};
  QCoreApplication app(argc, argv);

  Platform::ThreadRunner const threads;

  GMainLoop * loop = g_main_loop_new(nullptr, FALSE);
  std::atomic<bool> done = false;
  GetPlatform().RunTask(Platform::Thread::Background, [&done, loop]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [&done, loop]()
    {
      done = true;
      g_main_loop_quit(loop);
    });
  });
  guint const timeout = g_timeout_add_seconds(10, [](gpointer data) -> gboolean
  {
    g_main_loop_quit(static_cast<GMainLoop *>(data));
    return G_SOURCE_REMOVE;
  }, loop);

  g_main_loop_run(loop);
  if (done)
    g_source_remove(timeout);
  g_main_loop_unref(loop);
  TEST(done, ("The task posted from a worker thread did not run"));
}
}  // namespace gui_thread_tests
