#include "main_window.hpp"

#include "about.hpp"
#include "style.hpp"

#include "map/framework.hpp"

#include "platform/style_utils.hpp"

#include "indexer/map_style.hpp"

#include <cmath>

namespace gtk
{
namespace
{
char const * MyPositionIcon(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::PendingPosition: return "om-location-notfollow-symbolic";
  case location::NotFollowNoPosition: return "om-location-off-symbolic";
  case location::NotFollow: return "om-location-notfollow-symbolic";
  case location::Follow: return "om-location-follow-symbolic";
  case location::FollowAndRotate: return "om-location-rotate-symbolic";
  }
  return "om-location-notfollow-symbolic";
}

// The mode is not only shown by the icon and its colour.
char const * MyPositionLabel(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::PendingPosition: return "My Position: searching";
  case location::NotFollowNoPosition: return "My Position: no location";
  case location::NotFollow: return "My Position";
  case location::Follow: return "My Position: following";
  case location::FollowAndRotate: return "My Position: following and rotating the map";
  }
  return "My Position";
}
}  // namespace

MainWindow::MainWindow(AdwApplication * app)
{
  // Keep the object alive after the window is closed and destroyed by GTK.
  m_window = ADW_APPLICATION_WINDOW(g_object_ref(adw_application_window_new(GTK_APPLICATION(app))));
  gtk_window_set_title(GTK_WINDOW(m_window), "Organic Maps");
  gtk_window_set_icon_name(GTK_WINDOW(m_window), kAppId);
  gtk_window_set_default_size(GTK_WINDOW(m_window), 420, 860);

  auto * iconTheme = gtk_icon_theme_get_for_display(gdk_display_get_default());
  gtk_icon_theme_add_resource_path(iconTheme, "/app/organicmaps/gtk/icons");
  LoadCss();

  m_map = std::make_unique<MapView>();
  BuildUi();
  SetupFramework();

  Connect<void(GObject *, GParamSpec *)>(m_window, "notify::suspended",
                                         [this](GObject *, GParamSpec *) { OnSuspendedChanged(); });
  Connect<gboolean(GtkWindow *)>(m_window, "close-request", [this](GtkWindow *)
  {
    // Shut down while the widgets still exist: callbacks queued in the same main loop iteration
    // must not reach them after GTK destroys the window.
    Shutdown();
    return FALSE;
  });

  // The style manager outlives the window: disconnected in Shutdown().
  m_darkHandler = Connect<void(GObject *, GParamSpec *)>(adw_style_manager_get_default(), "notify::dark",
                                                         [this](GObject *, GParamSpec *) { SyncNightMode(); });
  SyncNightMode();
}

MainWindow::~MainWindow()
{
  Shutdown();
  m_map.reset();
  g_object_unref(m_window);
}

void MainWindow::Present()
{
  gtk_window_present(GTK_WINDOW(m_window));
}

void MainWindow::Shutdown()
{
  if (m_shutdown)
    return;
  m_shutdown = true;
  m_location.Stop();
  auto & fw = GetFramework();
  fw.SetMyPositionModeListener({});
  fw.SetPlacePageListeners({}, {}, {}, {});
  g_signal_handler_disconnect(adw_style_manager_get_default(), m_darkHandler);
  if (m_insetsIdle != 0)
    g_source_remove(m_insetsIdle);
  m_map->Shutdown();
  // Disposes the widgets, which disconnects their handlers. A closed window is destroyed already,
  // but not after g_application_quit().
  gtk_window_destroy(GTK_WINDOW(m_window));
}

void MainWindow::LoadCss()
{
  auto * provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider, kAppCss);
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
}

void MainWindow::BuildUi()
{
  m_toasts = ADW_TOAST_OVERLAY(adw_toast_overlay_new());

  auto * overlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(overlay), m_map->Widget());
  BuildMapControls(overlay);

  // Keep the map gestures separate from the standard draggable window header.
  auto * mapToolbar = adw_toolbar_view_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(mapToolbar), adw_header_bar_new());
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(mapToolbar), overlay);
  adw_toast_overlay_set_child(m_toasts, mapToolbar);

  adw_application_window_set_content(m_window, GTK_WIDGET(m_toasts));

  auto * keys = gtk_event_controller_key_new();
  Connect<gboolean(GtkEventControllerKey *, guint, guint, GdkModifierType)>(
      keys, "key-pressed", [](GtkEventControllerKey *, guint key, guint, GdkModifierType) -> gboolean
  {
    auto & fw = GetFramework();
    switch (key)
    {
    case GDK_KEY_plus:
    case GDK_KEY_equal:
    case GDK_KEY_KP_Add: fw.Scale(Framework::SCALE_MAG, true); return TRUE;
    case GDK_KEY_minus:
    case GDK_KEY_KP_Subtract: fw.Scale(Framework::SCALE_MIN, true); return TRUE;
    case GDK_KEY_Left: fw.Move(0.2, 0, true); return TRUE;
    case GDK_KEY_Right: fw.Move(-0.2, 0, true); return TRUE;
    case GDK_KEY_Up: fw.Move(0, -0.2, true); return TRUE;
    case GDK_KEY_Down: fw.Move(0, 0.2, true); return TRUE;
    default: return FALSE;
    }
  });
  gtk_widget_add_controller(GTK_WIDGET(m_window), keys);
}

void MainWindow::BuildMapControls(GtkWidget * overlay)
{
  // Zoom buttons: right side, vertically centred when there is enough height.
  m_rightButtons = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_halign(m_rightButtons, GTK_ALIGN_END);
  gtk_widget_set_valign(m_rightButtons, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_end(m_rightButtons, 12);
  auto * zoomIn = MakeIconButton("list-add-symbolic", "Zoom in", "map-button");
  auto * zoomOut = MakeIconButton("list-remove-symbolic", "Zoom out", "map-button");
  OnClicked(zoomIn, []() { GetFramework().Scale(Framework::SCALE_MAG, true); });
  OnClicked(zoomOut, []() { GetFramework().Scale(Framework::SCALE_MIN, true); });
  gtk_box_append(GTK_BOX(m_rightButtons), zoomIn);
  gtk_box_append(GTK_BOX(m_rightButtons), zoomOut);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), m_rightButtons);

  // Separate overlays sized to their content: a transparent container spanning the map would take
  // its input. UpdateMapInsets() stacks them above the bottom menu.
  m_myPositionButton = MakeIconButton(MyPositionIcon(location::PendingPosition), "My Position", "map-button");
  gtk_widget_set_halign(m_myPositionButton, GTK_ALIGN_END);
  gtk_widget_set_valign(m_myPositionButton, GTK_ALIGN_END);
  gtk_widget_set_margin_end(m_myPositionButton, 12);
  OnClicked(m_myPositionButton, [this]()
  {
    StartLocation();
    GetFramework().SwitchMyPositionNextMode();
  });
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), m_myPositionButton);

  // Keep map attribution readable and keyboard-accessible, with links to both sources.
  m_mapAttribution = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(m_mapAttribution),
                       "© <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors");
  gtk_label_set_wrap(GTK_LABEL(m_mapAttribution), TRUE);
  gtk_label_set_justify(GTK_LABEL(m_mapAttribution), GTK_JUSTIFY_CENTER);
  gtk_widget_set_halign(m_mapAttribution, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(m_mapAttribution, GTK_ALIGN_END);
  gtk_widget_add_css_class(m_mapAttribution, "caption");
  gtk_widget_add_css_class(m_mapAttribution, "map-attribution");
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), m_mapAttribution);

  m_bottomMenu = BuildBottomMenu();
  gtk_widget_set_valign(m_bottomMenu, GTK_ALIGN_END);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), m_bottomMenu);

  // Stack the bottom controls and let the engine know which parts of the map are covered (ruler and
  // compass placement).
  Connect<gboolean(GtkOverlay *, GtkWidget *, GdkRectangle *)>(overlay, "get-child-position",
                                                               [this](GtkOverlay *, GtkWidget *, GdkRectangle *)
  {
    QueueMapInsetsUpdate();
    return FALSE;
  });
}

void MainWindow::QueueMapInsetsUpdate()
{
  // Out of the allocation and snapshot passes, where the margins must not change.
  if (m_insetsIdle != 0)
    return;
  m_insetsIdle = g_idle_add_once([](gpointer data)
  {
    auto * self = static_cast<MainWindow *>(data);
    self->m_insetsIdle = 0;
    self->UpdateMapInsets();
  }, this);
}

GtkWidget * MainWindow::BuildBottomMenu()
{
  auto * bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(bar), TRUE);
  gtk_widget_add_css_class(bar, "bottom-menu");

  struct Item
  {
    char const * icon;
    char const * label;
    std::function<void()> action;
  };
  Item const items[] = {
      {"open-menu-symbolic", "Menu", [this]() { ShowMenu(); }},
  };
  for (auto const & item : items)
  {
    auto * button = gtk_button_new();
    auto * box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    auto * image = gtk_image_new_from_icon_name(item.icon);
    gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_LARGE);
    auto * label = gtk_label_new(item.label);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_add_css_class(label, "caption");
    gtk_box_append(GTK_BOX(box), image);
    gtk_box_append(GTK_BOX(box), label);
    gtk_button_set_child(GTK_BUTTON(button), box);
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_add_css_class(button, "bottom-menu-button");
    OnClicked(button, item.action);
    gtk_box_append(GTK_BOX(bar), button);
  }
  return bar;
}

void MainWindow::UpdateMapInsets()
{
  if (m_shutdown)
    return;
  // Border boxes, unlike gtk_widget_get_height(), include the CSS padding and border.
  auto const height = [this](GtkWidget * widget)
  {
    graphene_rect_t bounds;
    if (!gtk_widget_get_visible(widget) || !gtk_widget_compute_bounds(widget, m_map->Widget(), &bounds))
      return 0;
    return static_cast<int>(std::ceil(bounds.size.height));
  };
  int const menu = height(m_bottomMenu);
  int const attributionMargin = menu > 0 ? menu + 12 : 0;
  int const bottom = attributionMargin + height(m_mapAttribution);
  gtk_widget_set_margin_bottom(m_mapAttribution, attributionMargin);
  gtk_widget_set_margin_bottom(m_myPositionButton, bottom + 12);

  // Use the individual button heights so changing the box orientation cannot toggle the layout
  // back and forth. Keep separate overlays so the space beside the controls still takes map input.
  int const verticalZoomHeight = height(gtk_widget_get_first_child(m_rightButtons)) +
                                 gtk_box_get_spacing(GTK_BOX(m_rightButtons)) +
                                 height(gtk_widget_get_last_child(m_rightButtons));
  int const mapHeight = gtk_widget_get_height(m_map->Widget());
  int const positionTop = mapHeight - bottom - 12 - height(m_myPositionButton);
  bool const compact = (mapHeight + verticalZoomHeight) / 2 + 12 > positionTop;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(m_rightButtons),
                                 compact ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_valign(m_rightButtons, compact ? GTK_ALIGN_END : GTK_ALIGN_CENTER);
  gtk_widget_set_margin_bottom(m_rightButtons, compact ? bottom + 12 : 0);
  int minimum = 0, natural = 0;
  gtk_widget_measure(m_rightButtons, GTK_ORIENTATION_HORIZONTAL, -1, &minimum, &natural, nullptr, nullptr);
  // The measured width includes the zoom box's end margin.
  gtk_widget_set_margin_end(m_myPositionButton, compact ? natural + 12 : 12);

  if (bottom == m_lastBottomInset)
    return;
  m_lastBottomInset = bottom;
  m_map->SetInsets(0, bottom, 0, 0);
}

void MainWindow::SetupFramework()
{
  auto & fw = GetFramework();

  // Both are called on the main thread. Long tap toggles the map controls; the place page is not
  // implemented yet.
  fw.SetPlacePageListeners({}, {}, {}, [this]() { ToggleMapControls(); });
  fw.SetMyPositionModeListener([this](location::EMyPositionMode mode, bool) { OnMyPositionModeChanged(mode); });

  m_location.m_onLocation = [](location::GpsInfo const & info) { GetFramework().OnLocationUpdate(info); };
  m_location.m_onError = [this](location::TLocationError error)
  {
    GetFramework().OnLocationError(error);
    if (error == location::EDenied)
      Toast("Location services are disabled");
  };
  m_location.m_onCompass = [](double bearing)
  {
    location::CompassInfo info;
    info.m_bearing = bearing;
    GetFramework().OnCompassUpdate(info);
  };

  m_map->m_onEngineCreated = [this]()
  {
    // Like Android: ask for the location on start, the engine shows "pending" until the first fix.
    StartLocation();
    // Called from the snapshot of the map.
    QueueMapInsetsUpdate();
  };
}

void MainWindow::StartLocation()
{
  if (!m_location.IsStarted())
    m_location.Start();
}

void MainWindow::ToggleMapControls()
{
  // Long tap on the map hides/shows all controls, like on Android.
  m_controlsHidden = !m_controlsHidden;
  bool const show = !m_controlsHidden;
  gtk_widget_set_visible(m_bottomMenu, show);
  gtk_widget_set_visible(m_rightButtons, show);
  gtk_widget_set_visible(m_myPositionButton, show);
  QueueMapInsetsUpdate();
}

void MainWindow::OnMyPositionModeChanged(location::EMyPositionMode mode)
{
  gtk_button_set_icon_name(GTK_BUTTON(m_myPositionButton), MyPositionIcon(mode));
  gtk_widget_set_tooltip_text(m_myPositionButton, MyPositionLabel(mode));
  gtk_accessible_update_property(GTK_ACCESSIBLE(m_myPositionButton), GTK_ACCESSIBLE_PROPERTY_LABEL,
                                 MyPositionLabel(mode), -1);
  bool const active = mode == location::Follow || mode == location::FollowAndRotate;
  if (active)
    gtk_widget_add_css_class(m_myPositionButton, "following");
  else
    gtk_widget_remove_css_class(m_myPositionButton, "following");
  if (mode == location::PendingPosition)
    gtk_widget_add_css_class(m_myPositionButton, "pending");
  else
    gtk_widget_remove_css_class(m_myPositionButton, "pending");
}

void MainWindow::Toast(std::string const & text)
{
  auto * toast = adw_toast_new(text.c_str());
  adw_toast_set_timeout(toast, 3);
  adw_toast_overlay_add_toast(m_toasts, toast);
}

void MainWindow::ShowMenu()
{
  auto * dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Menu");
  adw_dialog_set_content_width(dialog, 420);

  auto * toolbar = adw_toolbar_view_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());

  auto * list = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(list, "boxed-list");
  SetMarginAll(list, 12);

  auto const addRow = [&](char const * icon, std::string const & title, std::function<void()> action)
  {
    auto * row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title.c_str());
    adw_action_row_add_prefix(ADW_ACTION_ROW(row), gtk_image_new_from_icon_name(icon));
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
    Connect<void(AdwActionRow *)>(row, "activated", [dialog, action](AdwActionRow *)
    {
      adw_dialog_close(dialog);
      action();
    });
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  };

  addRow("help-about-symbolic", "About Organic Maps", [this]() { ShowAboutDialog(*this); });

  auto * content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(content), list);
  auto * attribution = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(attribution),
                       "Map data © <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors");
  gtk_label_set_wrap(GTK_LABEL(attribution), TRUE);
  gtk_label_set_justify(GTK_LABEL(attribution), GTK_JUSTIFY_CENTER);
  gtk_widget_add_css_class(attribution, "caption");
  SetMarginAll(attribution, 12);
  gtk_box_append(GTK_BOX(content), attribution);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), content);
  adw_dialog_set_child(dialog, toolbar);
  adw_dialog_present(dialog, GTK_WIDGET(m_window));
}

void MainWindow::SyncNightMode()
{
  if (style_utils::GetNightModeSetting() != style_utils::NightMode::System)
    return;
  auto & fw = GetFramework();
  bool const dark = adw_style_manager_get_dark(adw_style_manager_get_default());
  auto const current = fw.GetMapStyle();
  auto const desired = dark ? GetDarkMapStyleVariant(current) : GetLightMapStyleVariant(current);
  if (desired != current)
    fw.SetMapStyle(desired);
}

void MainWindow::OnSuspendedChanged()
{
  bool const suspended = gtk_window_is_suspended(GTK_WINDOW(m_window));
  if (m_shutdown || suspended == m_suspended || !m_map->IsEngineCreated())
    return;
  m_suspended = suspended;
  if (suspended)
    GetFramework().EnterBackground();
  else
    GetFramework().EnterForeground();
}
}  // namespace gtk
