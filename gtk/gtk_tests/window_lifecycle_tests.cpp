#include "testing/testing.hpp"

#include "gtk/gtk_tests/fake_location_services.hpp"
#include "gtk/main_window.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <QtCore/QCoreApplication>

#include <adwaita.h>

#include <clocale>
#include <string>
#include <string_view>

namespace gtk
{
struct WindowTestAccess
{
  static uint64_t PresentedFrames(MainWindow const & window) { return window.m_map->m_presentedFrames.load(); }
};
}  // namespace gtk

namespace window_lifecycle_tests
{
using namespace gtk_test_support;

class CompositorInput
{
public:
  CompositorInput()
  {
    char const * compositor = g_getenv("OM_GTK_TEST_COMPOSITOR");
    CHECK(compositor, ("Run this test through CTest or run_window_tests.sh in a private D-Bus session"));
    m_connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    GVariant * owner = g_dbus_connection_call_sync(
        m_connection, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
        "GetConnectionUnixProcessID", g_variant_new("(s)", "org.gnome.Mutter.RemoteDesktop"), G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
    CHECK(owner, ("The isolated compositor is not available"));
    guint pid = 0;
    g_variant_get(owner, "(u)", &pid);
    g_variant_unref(owner);
    CHECK_EQUAL(pid, g_ascii_strtoull(compositor, nullptr, 10), ("Refusing to use another compositor"));
    GError * error = nullptr;
    GVariant * reply =
        g_dbus_connection_call_sync(m_connection, "org.gnome.Mutter.RemoteDesktop", "/org/gnome/Mutter/RemoteDesktop",
                                    "org.gnome.Mutter.RemoteDesktop", "CreateSession", nullptr, G_VARIANT_TYPE("(o)"),
                                    G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
    CHECK(reply, (error ? error->message : "Run this test through run_window_tests.sh"));
    char const * path;
    g_variant_get(reply, "(&o)", &path);
    m_path = path;
    g_variant_unref(reply);
    Call("Start", nullptr);
  }

  ~CompositorInput()
  {
    Call("Stop", nullptr);
    g_object_unref(m_connection);
  }

  void RestoreWindow()
  {
    // A minimized Wayland client cannot activate itself without user input. Alt+Tab restores it
    // through the isolated compositor, as a window manager would on a real desktop.
    Key(56 /* KEY_LEFTALT */, true);
    Key(15 /* KEY_TAB */, true);
    Key(15, false);
    Key(56, false);
  }

  void PressKey(guint key)
  {
    Key(key, true);
    Key(key, false);
  }

private:
  void Key(guint key, bool pressed) { Call("NotifyKeyboardKeycode", g_variant_new("(ub)", key, pressed)); }
  void Call(char const * method, GVariant * args)
  {
    GVariant * result = g_dbus_connection_call_sync(m_connection, "org.gnome.Mutter.RemoteDesktop", m_path.c_str(),
                                                    "org.gnome.Mutter.RemoteDesktop.Session", method, args, nullptr,
                                                    G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
    CHECK(result, (method));
    g_variant_unref(result);
    RunFor(30);
  }
  GDBusConnection * m_connection;
  std::string m_path;
};

GtkWidget * FindButton(GtkWidget * widget, std::string_view tooltip)
{
  auto const * text = gtk_widget_get_tooltip_text(widget);
  if (GTK_IS_BUTTON(widget) && text && std::string_view(text).starts_with(tooltip))
    return widget;
  for (auto * child = gtk_widget_get_first_child(widget); child; child = gtk_widget_get_next_sibling(child))
    if (auto * found = FindButton(child, tooltip))
      return found;
  return nullptr;
}

bool HasLabel(GtkWidget * button, char const * label)
{
  return g_strcmp0(gtk_widget_get_tooltip_text(button), label) == 0;
}

void Click(GtkWidget * button)
{
  g_signal_emit_by_name(button, "clicked");
}

void SaveScreenshot(gtk::MainWindow & window, std::string const & name)
{
  auto * root = GTK_WIDGET(window.Window());
  auto * paintable = gtk_widget_paintable_new(root);
  gtk_widget_queue_draw(root);
  RunFor(100);
  auto * snapshot = gtk_snapshot_new();
  gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot), gtk_widget_get_width(root), gtk_widget_get_height(root));
  auto * node = gtk_snapshot_free_to_node(snapshot);
  TEST(node, ());
  auto * renderer = gtk_native_get_renderer(GTK_NATIVE(root));
  auto * texture = gsk_renderer_render_texture(renderer, node, nullptr);
  TEST(gdk_texture_save_to_png(texture, GetPlatform().WritablePathForFile(name + ".png").c_str()), ());
  g_object_unref(texture);
  gsk_render_node_unref(node);
  g_object_unref(paintable);
}

void CheckControls(gtk::MainWindow & window, int width, int height)
{
  gtk_window_set_default_size(window.Window(), width, height);
  RunFor(200);
  auto * root = GTK_WIDGET(window.Window());
  for (char const * tooltip : {"My Position", "Zoom in", "Zoom out"})
  {
    auto * button = FindButton(root, tooltip);
    TEST(button, (tooltip));
    graphene_rect_t bounds;
    TEST(gtk_widget_compute_bounds(button, root, &bounds), (tooltip));
    TEST_GREATER_OR_EQUAL(bounds.origin.x, 0.0f, (tooltip));
    TEST_GREATER_OR_EQUAL(bounds.origin.y, 0.0f, (tooltip));
    TEST_LESS_OR_EQUAL(bounds.origin.x + bounds.size.width, gtk_widget_get_width(root), (tooltip));
    TEST_LESS_OR_EQUAL(bounds.origin.y + bounds.size.height, gtk_widget_get_height(root), (tooltip));
    auto * picked = gtk_widget_pick(root, bounds.origin.x + bounds.size.width / 2,
                                    bounds.origin.y + bounds.size.height / 2, GTK_PICK_DEFAULT);
    TEST(picked, (tooltip));
    TEST(picked == button || gtk_widget_is_ancestor(picked, button), (tooltip));
  }
}

// This exercises the real window, its mode listener, drape and the async providers. Run in a
// compositor: gtk_window_tests has a separate CTest label and a nested Mutter runner.
UNIT_TEST(Window_LocationAndRenderingLifecycle)
{
  CompositorInput input;
  // The runner already supplies a private bus shared with the isolated compositor.
  FakeServices services(false /* privateBus */);
  int argc = 1;
  char arg0[] = "gtk_window_tests";
  char * argv[] = {arg0, nullptr};
  QCoreApplication qtApp(argc, argv);
  adw_init();
  std::setlocale(LC_NUMERIC, "C");
  settings::Delete("LastLocationStateMode");
  style_utils::SetNightModeSetting(style_utils::NightMode::System);
  Framework framework(FrameworkParams{});
  gtk::GetApp().framework = &framework;
  auto * app = adw_application_new("app.organicmaps.gtk.tests", G_APPLICATION_NON_UNIQUE);
  TEST(g_application_register(G_APPLICATION(app), nullptr, nullptr), ());
  {
    gtk::MainWindow window(app);
    window.Present();
    TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
    auto * button = FindButton(GTK_WIDGET(window.Window()), "My Position");
    TEST(button, ());

    // Cancel the first request through the actual button, then publish its late first fix.
    Click(button);
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: no location"); }), ());
    services.PublishFix();
    RunFor(200);
    TEST(HasLabel(button, "My Position: no location"), ());
    TEST_EQUAL(services.Count("Start"), 1, ());

    Click(button);
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: following"); }), (services.Calls()));
    TEST_EQUAL(services.Count("Start"), 2, ());

    // Deactivation after a fix must allow one explicit retry, without an automatic retry loop.
    services.SetActive(false);
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: searching"); }), ());
    RunFor(100);
    TEST_EQUAL(services.Count("Start"), 2, ());
    services.SetActive(true);
    Click(button);
    TEST(RunUntil([&]() { return services.Count("Start") == 3 && HasLabel(button, "My Position: following"); }),
         (services.Calls()));

    // Resume also permits a retry after failure when the position mode is still enabled.
    services.SetActive(false);
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: searching"); }), ());
    gtk_window_minimize(window.Window());
    TEST(RunUntil([&]() { return gtk_window_is_suspended(window.Window()); }), ());
    services.SetActive(true);
    input.RestoreWindow();
    TEST(RunUntil([&]() { return !gtk_window_is_suspended(window.Window()) && services.Count("Start") == 4; }),
         (gtk_window_is_suspended(window.Window()), services.Calls()));
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: following"); }), ());

    auto * mapArea = gtk_widget_get_first_child(GTK_WIDGET(window.Window()));
    std::function<GtkWidget *(GtkWidget *)> findArea = [&](GtkWidget * widget) -> GtkWidget *
    {
      if (GTK_IS_GL_AREA(widget))
        return widget;
      for (auto * child = gtk_widget_get_first_child(widget); child; child = gtk_widget_get_next_sibling(child))
        if (auto * area = findArea(child))
          return area;
      return nullptr;
    };
    mapArea = findArea(mapArea);
    TEST(mapArea, ());
    struct RenderObserver
    {
      GtkWidget * m_area;
      int m_frames = 0;
    } observer{mapArea};
    guint const signal = g_signal_lookup("render", GTK_TYPE_GL_AREA);
    gulong const hook = g_signal_add_emission_hook(
        signal, 0, +[](GSignalInvocationHint *, guint, GValue const * values, gpointer data) -> gboolean
    {
      auto & o = *static_cast<RenderObserver *>(data);
      if (g_value_get_object(&values[0]) == o.m_area)
        ++o.m_frames;
      return TRUE;
    }, &observer, nullptr);
    SCOPE_GUARD(removeHook, [=]() { g_signal_remove_emission_hook(signal, hook); });

    auto produced = [&]() { return gtk::WindowTestAccess::PresentedFrames(window); };
    auto requireProgress = [&]()
    {
      auto const before = produced();
      int const rendered = observer.m_frames;
      framework.InvalidateRendering();
      TEST(RunUntil([&]() { return produced() > before && observer.m_frames > rendered; }),
           (before, produced(), rendered, observer.m_frames));
    };
    for (int i = 0; i < 2; ++i)
    {
      requireProgress();
      int const releases = services.Count("ReleaseCompass");
      gtk_window_minimize(window.Window());
      TEST(RunUntil([&]()
      { return gtk_window_is_suspended(window.Window()) && services.Count("ReleaseCompass") > releases; }),
           (services.Calls()));
      auto const pausedFrames = produced();
      services.SetHeading(45 + i);
      // Force work even while the compositor does not draw this surface. An idle scene cannot
      // conceal a missing SetRenderingDisabled(): the producer must remain stopped.
      for (int j = 0; j < 5; ++j)
      {
        framework.InvalidateRendering();
        RunFor(100);
      }
      TEST_EQUAL(produced(), pausedFrames, ("Drape continued producing frames while suspended"));
      TEST_EQUAL(services.Count("Start"), 4 + i, ());
      input.RestoreWindow();
      TEST(RunUntil([&]() { return !gtk_window_is_suspended(window.Window()) && services.Count("Start") == 5 + i; }),
           (services.Calls()));
      requireProgress();
    }

    // F10 is provided by the primary menu button, including when the map owns keyboard focus.
    gtk_widget_grab_focus(mapArea);
    input.PressKey(68 /* KEY_F10 */);
    std::function<GtkMenuButton *(GtkWidget *)> findMenu = [&](GtkWidget * widget) -> GtkMenuButton *
    {
      if (GTK_IS_MENU_BUTTON(widget))
        return GTK_MENU_BUTTON(widget);
      for (auto * child = gtk_widget_get_first_child(widget); child; child = gtk_widget_get_next_sibling(child))
        if (auto * menu = findMenu(child))
          return menu;
      return nullptr;
    };
    auto * menu = findMenu(GTK_WIDGET(window.Window()));
    TEST(menu && gtk_menu_button_get_primary(menu), ());
    auto * popover = gtk_menu_button_get_popover(menu);
    TEST(RunUntil([&]() { return gtk_widget_get_visible(GTK_WIDGET(popover)); }), ());
    input.PressKey(1 /* KEY_ESC */);
    TEST(RunUntil([&]() { return !gtk_widget_get_visible(GTK_WIDGET(popover)); }), ());
    input.PressKey(68 /* KEY_F10 */);
    TEST(RunUntil([&]() { return gtk_widget_get_visible(GTK_WIDGET(popover)); }), ());
    input.PressKey(28 /* KEY_ENTER */);
    TEST(RunUntil(
             [&]()
    {
      auto * dialog = adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(window.Window()));
      return dialog && ADW_IS_ABOUT_DIALOG(dialog);
    }),
         ());
    adw_dialog_close(adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(window.Window())));

    framework.Scale(0.0001, false /* isAnim */);
    RunFor(500);
    auto * style = adw_style_manager_get_default();
    auto * gtkSettings = gtk_settings_get_default();
    char * originalFont = nullptr;
    g_object_get(gtkSettings, "gtk-font-name", &originalFont, nullptr);
    SCOPE_GUARD(restoreFont, [&]()
    {
      g_object_set(gtkSettings, "gtk-font-name", originalFont, nullptr);
      g_free(originalFont);
    });
    for (auto scheme : {ADW_COLOR_SCHEME_FORCE_LIGHT, ADW_COLOR_SCHEME_FORCE_DARK})
    {
      adw_style_manager_set_color_scheme(style, scheme);
      for (char const * font : {"Sans 11", "Sans 18"})
      {
        g_object_set(gtkSettings, "gtk-font-name", font, nullptr);
        CheckControls(window, 420, 860);
        SaveScreenshot(window, std::string(scheme == ADW_COLOR_SCHEME_FORCE_DARK ? "dark-" : "light-") + font);
        CheckControls(window, 850, 420);
        CheckControls(window, 860, 360);
        CheckControls(window, 360, 294);
        SaveScreenshot(window,
                       std::string(scheme == ADW_COLOR_SCHEME_FORCE_DARK ? "compact-dark-" : "compact-light-") + font);
      }
    }
    window.ToggleMapControls();
    RunFor(100);
    window.ToggleMapControls();
    CheckControls(window, 420, 860);
    TEST(gtk_widget_activate_action(GTK_WIDGET(window.Window()), "win.about", nullptr), ());
    RunFor(100);
    auto * dialog = adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(window.Window()));
    TEST(dialog && ADW_IS_ABOUT_DIALOG(dialog), ());
    adw_dialog_close(dialog);

    gtk_window_minimize(window.Window());
    TEST(RunUntil([&]() { return gtk_window_is_suspended(window.Window()); }), ());
    window.Shutdown();
    services.PublishFix();
    services.SetHeading(180);
    RunFor(100);
  }
  RunFor(100);
  services.Reset();
  settings::Set("LastLocationStateMode", location::NotFollowNoPosition);
  settings::Set("LastEnterBackground", base::Timer::LocalTime());
  {
    gtk::MainWindow disabled(app);
    disabled.Present();
    auto * button = FindButton(GTK_WIDGET(disabled.Window()), "My Position");
    TEST(button, ());
    TEST(RunUntil([&]() { return HasLabel(button, "My Position: no location"); }), ());
    RunFor(100);
    TEST_EQUAL(services.Count("Start"), 0, ());
    gtk_window_minimize(disabled.Window());
    TEST(RunUntil([&]() { return gtk_window_is_suspended(disabled.Window()); }), ());
    input.RestoreWindow();
    TEST(RunUntil([&]() { return !gtk_window_is_suspended(disabled.Window()); }), ());
    RunFor(100);
    TEST_EQUAL(services.Count("Start"), 0, ());
    disabled.Shutdown();
  }
  framework.PrepareToShutdown();
  RunFor(100);
  gtk::GetApp().framework = nullptr;
  g_object_unref(app);
}
}  // namespace window_lifecycle_tests
