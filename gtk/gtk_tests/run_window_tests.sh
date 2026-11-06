#!/usr/bin/env bash
set -euo pipefail

export WAYLAND_DISPLAY="organicmaps-gtk-tests-$$"
export GDK_BACKEND=wayland
export GTK_A11Y=none
export GSK_RENDERER=gl
export GIO_USE_VFS=local
export GSETTINGS_BACKEND=memory

test_runtime=""
if [[ -z "${XDG_RUNTIME_DIR:-}" ]]; then
  test_runtime=$(mktemp -d)
  export XDG_RUNTIME_DIR="$test_runtime"
fi
unset DISPLAY

mutter --headless --no-x11 --virtual-monitor=1600x1200 --wayland-display="$WAYLAND_DISPLAY" \
  > gtk-window-mutter.log 2>&1 &
compositor_pid=$!
export OM_GTK_TEST_COMPOSITOR="$compositor_pid"
cleanup() {
  kill "$compositor_pid" 2>/dev/null || true
  wait "$compositor_pid" 2>/dev/null || true
  if [[ -n "$test_runtime" ]]; then
    rm -rf -- "$test_runtime"
  fi
}
trap cleanup EXIT
for ((attempt = 0; attempt < 100; ++attempt)); do
  if [[ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]] &&
     gdbus wait --session --timeout 1 org.gnome.Mutter.RemoteDesktop; then
    if "$@" > gtk-window-client.log 2>&1; then
      cat gtk-window-client.log
      exit 0
    else
      test_status=$?
      cat gtk-window-client.log
      exit "$test_status"
    fi
  fi
  if ! kill -0 "$compositor_pid" 2>/dev/null; then
    cat gtk-window-mutter.log
    exit 1
  fi
  sleep 0.1
done
cat gtk-window-mutter.log
exit 1
