#pragma once

#include <adwaita.h>
#include <gtk/gtk.h>

#include <functional>

class Framework;

namespace gtk
{
// Application ID: D-Bus name, desktop file and icon name.
inline constexpr char kAppId[] = "app.organicmaps.gtk";

class MainWindow;

// Global application state. Everything here lives on the GTK main thread.
struct App
{
  Framework * framework = nullptr;
  AdwApplication * application = nullptr;
  MainWindow * window = nullptr;
};

App & GetApp();
Framework & GetFramework();

// Small helpers around plain GTK C API.
GtkWidget * MakeIconButton(char const * iconName, char const * tooltip, char const * cssClass = nullptr);

// Connects a lambda to a GObject signal with no extra arguments besides the instance.
template <typename Fn>
void OnClicked(GtkWidget * button, Fn && fn)
{
  auto * heap = new std::function<void()>(std::forward<Fn>(fn));
  g_signal_connect_data(
      button, "clicked",
      G_CALLBACK(+[](GtkWidget *, gpointer data) { (*static_cast<std::function<void()> *>(data))(); }), heap,
      [](gpointer data, GClosure *) { delete static_cast<std::function<void()> *>(data); }, GConnectFlags(0));
}

// Generic signal connection: Connect<void(GtkWidget *, int)>(obj, "signal", lambda).
// The std::function signature must match the C signal handler without the trailing user_data.
template <typename R, typename... Args>
struct SignalThunk
{
  static R Call(Args... args, gpointer data) { return (*static_cast<std::function<R(Args...)> *>(data))(args...); }
};

template <typename R, typename... Args>
gulong ConnectImpl(gpointer instance, char const * signal, std::function<R(Args...)> && fn)
{
  auto * heap = new std::function<R(Args...)>(std::move(fn));
  auto const thunk = &SignalThunk<R, Args...>::Call;
  return g_signal_connect_data(instance, signal, G_CALLBACK(thunk), heap, [](gpointer data, GClosure *)
  { delete static_cast<std::function<R(Args...)> *>(data); }, GConnectFlags(0));
}

template <typename Signature, typename Fn>
gulong Connect(gpointer instance, char const * signal, Fn && fn)
{
  return ConnectImpl(instance, signal, std::function<Signature>(std::forward<Fn>(fn)));
}
}  // namespace gtk
