#pragma once

#include "gl_factory.hpp"
#include "touch_input.hpp"

#include <gtk/gtk.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

namespace gtk
{
// The map: a GtkGLArea that composes frames produced by the drape render thread and feeds raw
// touches to the engine (it recognizes taps, long taps, drags, pinch and rotation itself, like on
// Android).
class MapView
{
public:
  MapView();
  ~MapView();

  GtkWidget * Widget() const { return m_area; }

  // Device pixels per logical pixel.
  double Scale() const { return m_scale; }
  bool IsEngineCreated() const { return m_engineCreated; }

  // Logical pixels occupied by GTK attribution below the ruler and visible map viewport.
  void SetBottomInset(int bottom);

  // Called once the drape engine exists.
  std::function<void()> m_onEngineCreated;

  // Destroys the engine; nothing it has queued on the main loop runs afterwards.
  void Shutdown();

private:
#ifdef OM_GTK_WINDOW_TESTS
  friend struct WindowTestAccess;
  std::atomic<uint64_t> m_presentedFrames = 0;
#endif
  void OnRealize();
  void OnUnrealize();
  void OnResize(int w, int h);
  gboolean OnRender(GdkGLContext * ctx);
  gboolean OnEvent(GdkEvent * event);
  void OnScroll(double dx, double dy, GdkScrollUnit unit);
  void CreateEngine(int w, int h);
  void BuildBlitProgram();
  void UpdateWidgetLayout();
  m2::PointF ToDevice(double x, double y) const;
  void QueueRender();

  GtkWidget * m_area = nullptr;
  std::unique_ptr<GtkContextFactory> m_factory;
  bool m_engineCreated = false;
  bool m_shutdown = false;
  double m_scale = 1.0;
  int m_pixelWidth = 0;
  int m_pixelHeight = 0;
  int m_bottomInset = 0;

  GLuint m_program = 0;
  GLuint m_vao = 0;
  GLuint m_vbo = 0;
  GLint m_samplerLoc = -1;

  std::atomic<bool> m_renderQueued = false;

  TouchInput m_touches;
  // The last pointer position over the map in logical px: the scale may change before the next motion.
  std::optional<m2::PointD> m_pointer;
  double m_pinchScale = 1.0;
};
}  // namespace gtk
