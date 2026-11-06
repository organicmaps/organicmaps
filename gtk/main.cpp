#include "app.hpp"
#include "main_window.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/platform_linux_migration.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "base/logging.hpp"

#include <QtCore/QCoreApplication>

#include <glib-unix.h>

#include <clocale>
#include <csignal>

namespace
{
void OnStartup(GApplication *, gpointer)
{
  using namespace gtk;
  // gtk_init() resets the locale; the core parses doubles with a dot.
  std::setlocale(LC_NUMERIC, "C");

  // A GNOME app follows the system dark style unless the user chose otherwise.
  if (int v = 0; !settings::Get("NightMode", v))
    style_utils::SetNightModeSetting(style_utils::NightMode::System);

  auto * fw = new Framework(FrameworkParams{});
  fw->SetupMeasurementSystem();
  GetApp().framework = fw;

  GetApp().window = new MainWindow(GetApp().application);
}

void OnActivate(GApplication *, gpointer)
{
  if (auto * w = gtk::GetApp().window)
    w->Present();
}

void OnShutdown(GApplication *, gpointer)
{
  // GApplication still dispatches pending sources after "shutdown": the objects are deleted when
  // g_application_run() returns.
  if (auto * w = gtk::GetApp().window)
    w->Shutdown();
}
}  // namespace

int main(int argc, char * argv[])
{
  std::setlocale(LC_NUMERIC, "C");

  // The core posts its UI-thread tasks through Qt; Qt's GLib event dispatcher runs them from the
  // GTK main loop, so QCoreApplication::exec() is never called.
  int qtArgc = 1;
  char * qtArgv[] = {argv[0], nullptr};
  QCoreApplication qtApp(qtArgc, qtArgv);
  QCoreApplication::setApplicationName("Organic Maps");
  platform::EnableDesktopDataMigration();

  Platform & platform = GetPlatform();
  LOG(LINFO,
      ("Organic Maps", platform.Version(), "resources:", platform.ResourcesDir(), "writable:", platform.WritableDir()));

  auto * app = adw_application_new(gtk::kAppId, G_APPLICATION_DEFAULT_FLAGS);
  gtk::GetApp().application = app;
  g_signal_connect(app, "startup", G_CALLBACK(OnStartup), nullptr);
  g_signal_connect(app, "activate", G_CALLBACK(OnActivate), nullptr);
  g_signal_connect(app, "shutdown", G_CALLBACK(OnShutdown), nullptr);

  // Close gracefully on SIGTERM/SIGINT too: saves the viewport.
  for (int sig : {SIGTERM, SIGINT, SIGHUP})
    g_unix_signal_add(sig, [](gpointer a) -> gboolean
    {
      g_application_quit(G_APPLICATION(a));
      return G_SOURCE_REMOVE;
    }, app);

  int const status = g_application_run(G_APPLICATION(app), argc, argv);
  auto & state = gtk::GetApp();
  delete state.window;
  state.window = nullptr;
  if (state.framework)
  {
    state.framework->PrepareToShutdown();
    delete state.framework;
    state.framework = nullptr;
  }
  g_object_unref(app);
  LOG(LINFO, ("Organic Maps finished with code", status));
  return status;
}
