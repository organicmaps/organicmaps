#include "map_view.hpp"

#include "app.hpp"

#include "map/framework.hpp"

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/visual_params.hpp"

#include "base/logging.hpp"

#include <cmath>

namespace gtk
{
namespace
{
char const * kVertexShader = R"(#version 300 es
in vec4 a_position;
out vec2 v_uv;
void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = a_position.zw;
}
)";

char const * kFragmentShader = R"(#version 300 es
precision mediump float;
in vec2 v_uv;
uniform sampler2D u_sampler;
out vec4 v_color;
void main()
{
  v_color = vec4(texture(u_sampler, v_uv).rgb, 1.0);
}
)";

GLuint CompileShader(GLenum type, char const * src)
{
  GLuint const shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    char log[1024] = {};
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    LOG(LERROR, ("Blit shader compilation failed:", log));
  }
  return shader;
}
}  // namespace

MapView::MapView()
  : m_touches([this](df::TouchEvent const & e)
{
  if (m_engineCreated)
    GetFramework().TouchEvent(e);
})
{
  m_area = gtk_gl_area_new();
  g_object_ref_sink(m_area);
  gtk_widget_set_hexpand(m_area, TRUE);
  gtk_widget_set_vexpand(m_area, TRUE);
  gtk_widget_set_focusable(m_area, TRUE);
  gtk_widget_add_css_class(m_area, "map-view");
  gtk_accessible_update_property(GTK_ACCESSIBLE(m_area), GTK_ACCESSIBLE_PROPERTY_LABEL, "Map",
                                 GTK_ACCESSIBLE_PROPERTY_DESCRIPTION,
                                 "Arrow keys move the map, plus and minus keys zoom it", -1);

  auto * area = GTK_GL_AREA(m_area);
  gtk_gl_area_set_allowed_apis(area, GDK_GL_API_GLES);
  gtk_gl_area_set_required_version(area, 3, 0);
  gtk_gl_area_set_has_depth_buffer(area, FALSE);
  gtk_gl_area_set_has_stencil_buffer(area, FALSE);
  gtk_gl_area_set_auto_render(area, FALSE);

  Connect<void(GtkWidget *)>(m_area, "realize", [this](GtkWidget *) { OnRealize(); });
  Connect<void(GtkWidget *)>(m_area, "unrealize", [this](GtkWidget *) { OnUnrealize(); });
  Connect<void(GtkGLArea *, int, int)>(m_area, "resize", [this](GtkGLArea *, int w, int h) { OnResize(w, h); });
  Connect<gboolean(GtkGLArea *, GdkGLContext *)>(m_area, "render",
                                                 [this](GtkGLArea *, GdkGLContext * c) { return OnRender(c); });

  auto * legacy = gtk_event_controller_legacy_new();
  Connect<gboolean(GtkEventControllerLegacy *, GdkEvent *)>(
      legacy, "event", [this](GtkEventControllerLegacy *, GdkEvent * e) { return OnEvent(e); });
  gtk_widget_add_controller(m_area, legacy);

  // Scroll events have no position on Wayland: track the pointer to zoom around it.
  auto * motion = gtk_event_controller_motion_new();
  for (char const * signal : {"enter", "motion"})
  {
    Connect<void(GtkEventControllerMotion *, double, double)>(
        motion, signal, [this](GtkEventControllerMotion *, double x, double y) { m_pointer.emplace(x, y); });
  }
  gtk_widget_add_controller(m_area, motion);

  auto * scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  Connect<gboolean(GtkEventControllerScroll *, double, double)>(
      scroll, "scroll", [this, scroll](GtkEventControllerScroll *, double dx, double dy)
  {
    OnScroll(dx, dy, gtk_event_controller_scroll_get_unit(GTK_EVENT_CONTROLLER_SCROLL(scroll)));
    return TRUE;
  });
  gtk_widget_add_controller(m_area, scroll);

  // Touchpad pinch on laptops.
  auto * zoom = gtk_gesture_zoom_new();
  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(zoom), GTK_PHASE_BUBBLE);
  Connect<void(GtkGestureZoom *, double)>(zoom, "scale-changed", [this, zoom](GtkGestureZoom *, double scale)
  {
    // Touchscreen pinches are handled by the engine through raw touches. Check the event, not the
    // device: on Wayland touchpad gestures come from the logical pointer with the mouse source.
    auto * event = gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(zoom));
    if (!m_engineCreated || !event || gdk_event_get_event_type(event) != GDK_TOUCHPAD_PINCH)
      return;
    GetFramework().Scale(scale / m_pinchScale, false);
    m_pinchScale = scale;
  });
  Connect<void(GtkGesture *, GdkEventSequence *)>(zoom, "begin",
                                                  [this](GtkGesture *, GdkEventSequence *) { m_pinchScale = 1.0; });
  gtk_widget_add_controller(m_area, GTK_EVENT_CONTROLLER(zoom));
}

MapView::~MapView()
{
  Shutdown();
  g_object_unref(m_area);
}

void MapView::Shutdown()
{
  if (m_shutdown)
    return;
  m_shutdown = true;
  if (m_engineCreated)
  {
    auto & fw = GetFramework();
    fw.EnterBackground();
    fw.SetRenderingDisabled(true);
    m_factory->PrepareToShutdown();
    // Joins the render thread, so QueueRender() is not called any more.
    fw.DestroyDrapeEngine();
    m_engineCreated = false;
  }
  // The idle callback of QueueRender() may still be pending.
  g_idle_remove_by_data(this);
  m_factory.reset();
}

void MapView::OnRealize()
{
  gtk_gl_area_make_current(GTK_GL_AREA(m_area));
  if (auto * err = gtk_gl_area_get_error(GTK_GL_AREA(m_area)))
  {
    LOG(LERROR, ("GtkGLArea error:", err->message));
    return;
  }
  BuildBlitProgram();
}

void MapView::OnUnrealize()
{
  m_touches.Cancel();
  gtk_gl_area_make_current(GTK_GL_AREA(m_area));
  if (m_program)
    glDeleteProgram(m_program);
  if (m_vbo)
    glDeleteBuffers(1, &m_vbo);
  if (m_vao)
    glDeleteVertexArrays(1, &m_vao);
  m_program = m_vbo = m_vao = 0;
}

void MapView::BuildBlitProgram()
{
  GLuint const vs = CompileShader(GL_VERTEX_SHADER, kVertexShader);
  GLuint const fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader);
  m_program = glCreateProgram();
  glAttachShader(m_program, vs);
  glAttachShader(m_program, fs);
  glBindAttribLocation(m_program, 0, "a_position");
  glLinkProgram(m_program);
  glDeleteShader(vs);
  glDeleteShader(fs);
  m_samplerLoc = glGetUniformLocation(m_program, "u_sampler");

  // Triangle strip: position xy + texture uv.
  float const vertices[] = {-1.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 0.f};
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MapView::OnResize(int w, int h)
{
  if (w <= 0 || h <= 0 || m_shutdown)
    return;
  double const oldScale = m_scale;
  int const logicalW = gtk_widget_get_width(m_area);
  if (logicalW > 0)
    m_scale = static_cast<double>(w) / logicalW;
  m_pixelWidth = w;
  m_pixelHeight = h;

  if (!m_engineCreated)
  {
    CreateEngine(w, h);
    return;
  }
  auto & fw = GetFramework();
  // E.g. the window moved to a monitor with another scale. The tolerance ignores pixel rounding:
  // the engine reloads its resources on every change. Its drag threshold keeps the initial scale.
  if (std::abs(m_scale - oldScale) > 0.01)
  {
    LOG(LINFO, ("Visual scale changed from", oldScale, "to", m_scale));
    // The gesture in progress has its points in the old device pixels.
    m_touches.Cancel();
    fw.UpdateVisualScale(m_scale);
  }
  fw.OnSize(w, h);
  UpdateWidgetLayout();
}

void MapView::CreateEngine(int w, int h)
{
  m_factory = std::make_unique<GtkContextFactory>(gtk_widget_get_display(m_area), w, h, [this]() { QueueRender(); });
  if (!m_factory->IsValid())
  {
    LOG(LERROR, ("Can't create OpenGL ES 3 contexts for the map"));
    m_factory.reset();
    return;
  }

  Framework::DrapeCreationParams p;
  p.m_apiVersion = dp::ApiVersion::OpenGLES3;
  p.m_surfaceWidth = w;
  p.m_surfaceHeight = h;
  p.m_visualScale = m_scale;

  // Only the initial placement: UpdateWidgetLayout() moves the widgets later.
  gui::Skin skin(gui::ResolveGuiSkinFile("default"), m_scale);
  skin.Resize(w, h);
  skin.ForEach([&p](gui::EWidget widget, gui::Position const & pos)
  {
    // MainWindow provides persistent, clickable GTK attribution instead of the fading GL label.
    if (widget != gui::WIDGET_COPYRIGHT)
      p.m_widgetsInitInfo[widget] = pos;
  });
  p.m_widgetsInitInfo[gui::WIDGET_SCALE_FPS_LABEL] = gui::Position(dp::LeftTop);

  auto & fw = GetFramework();
  LOG(LINFO, ("Creating drape engine", w, "x", h, "visual scale", m_scale));
  fw.CreateDrapeEngine(make_ref(m_factory.get()), std::move(p));
  fw.EnterForeground();
  m_engineCreated = true;
  UpdateWidgetLayout();

  if (m_onEngineCreated)
    m_onEngineCreated();
}

void MapView::SetInsets(int top, int bottom, int left, int right)
{
  m_insets = {top, bottom, left, right};
  UpdateWidgetLayout();
}

void MapView::UpdateWidgetLayout()
{
  if (!m_engineCreated || m_pixelWidth == 0)
    return;
  double const s = m_scale;
  double const margin = 12 * s;
  double const top = m_insets[0] * s;
  double const bottom = m_insets[1] * s;

  gui::TWidgetsLayoutInfo layout;
  // Ruler: bottom-left above the bottom panels; compass: top-right below the top ones.
  layout[gui::WIDGET_RULER] = m2::PointD(margin, m_pixelHeight - bottom - margin);
  layout[gui::WIDGET_COMPASS] = m2::PointD(m_pixelWidth - margin - 24 * s, top + margin + 24 * s);
  auto & fw = GetFramework();
  fw.SetWidgetLayout(std::move(layout));

  // The part of the map not covered by panels: used to centre the position arrow and selections.
  fw.SetVisibleViewport(m2::RectD(0, top, m_pixelWidth, std::max(top + 1, m_pixelHeight - bottom)));
}

gboolean MapView::OnRender(GdkGLContext *)
{
  m_renderQueued = false;
  bool const drawn = m_program != 0 && m_factory &&
                     m_factory->DrawFrame([this](GLuint texture)
  {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(m_samplerLoc, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  });
  if (!drawn)
  {
    if (adw_style_manager_get_dark(adw_style_manager_get_default()))
      glClearColor(0.13f, 0.13f, 0.14f, 1.f);
    else
      glClearColor(0.93f, 0.93f, 0.91f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  return TRUE;
}

void MapView::QueueRender()
{
  // Called on the render thread when a frame is ready.
  if (m_renderQueued.exchange(true))
    return;
  g_idle_add_full(G_PRIORITY_DEFAULT, [](gpointer data) -> gboolean
  {
    auto * self = static_cast<MapView *>(data);
    if (!self->m_shutdown)
      gtk_gl_area_queue_render(GTK_GL_AREA(self->m_area));
    else
      self->m_renderQueued = false;
    return G_SOURCE_REMOVE;
  }, this, nullptr);
}

m2::PointF MapView::ToDevice(double x, double y) const
{
  return m2::PointF(static_cast<float>(x * m_scale), static_cast<float>(y * m_scale));
}

gboolean MapView::OnEvent(GdkEvent * event)
{
  if (!m_engineCreated)
    return FALSE;

  auto const type = gdk_event_get_event_type(event);
  bool const isTouch =
      type == GDK_TOUCH_BEGIN || type == GDK_TOUCH_UPDATE || type == GDK_TOUCH_END || type == GDK_TOUCH_CANCEL;
  bool const isMouse = type == GDK_BUTTON_PRESS || type == GDK_BUTTON_RELEASE || type == GDK_MOTION_NOTIFY;
  if (!isTouch && !isMouse)
    return FALSE;

  if (type == GDK_TOUCH_CANCEL)
  {
    m_touches.TouchCancel(gdk_event_get_event_sequence(event));
    return TRUE;
  }

  // Legacy controllers get surface coordinates: translate to the widget.
  double sx = 0, sy = 0;
  if (!gdk_event_get_position(event, &sx, &sy))
    return FALSE;
  auto * native = gtk_widget_get_native(m_area);
  double nx = 0, ny = 0;
  gtk_native_get_surface_transform(native, &nx, &ny);
  graphene_point_t const in = GRAPHENE_POINT_INIT(static_cast<float>(sx - nx), static_cast<float>(sy - ny));
  graphene_point_t out;
  if (!gtk_widget_compute_point(GTK_WIDGET(native), m_area, &in, &out))
    return FALSE;
  m2::PointF const pos = ToDevice(out.x, out.y);

  if (isTouch)
  {
    void const * seq = gdk_event_get_event_sequence(event);
    if (type == GDK_TOUCH_BEGIN && m_touches.TouchBegin(seq, pos))
      gtk_widget_grab_focus(m_area);
    else if (type == GDK_TOUCH_UPDATE)
      m_touches.TouchUpdate(seq, pos);
    else if (type == GDK_TOUCH_END)
      m_touches.TouchEnd(seq, pos);
    return TRUE;
  }

  // Pointer events emulated from the touchscreen: the touches are handled above.
  if (auto * device = gdk_event_get_device(event); device && gdk_device_get_source(device) == GDK_SOURCE_TOUCHSCREEN)
    return FALSE;

  switch (type)
  {
  case GDK_BUTTON_PRESS:
    if (gdk_button_event_get_button(event) != GDK_BUTTON_PRIMARY || !m_touches.MouseDown(pos))
      return FALSE;
    gtk_widget_grab_focus(m_area);
    return TRUE;
  case GDK_MOTION_NOTIFY: return m_touches.MouseMove(pos);
  case GDK_BUTTON_RELEASE: return gdk_button_event_get_button(event) == GDK_BUTTON_PRIMARY && m_touches.MouseUp(pos);
  default: return FALSE;
  }
}

void MapView::OnScroll(double dx, double dy, GdkScrollUnit unit)
{
  if (!m_engineCreated)
    return;
  UNUSED_VALUE(dx);
  double const steps = unit == GDK_SCROLL_UNIT_WHEEL ? dy : dy / 40.0;
  double const factor = std::exp(-steps * 0.25);
  if (m_pointer)
    GetFramework().Scale(factor, m2::PointD(ToDevice(m_pointer->x, m_pointer->y)), false);
  else
    GetFramework().Scale(factor, false);
}
}  // namespace gtk
