package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface RoutingListener
{
  /** Called from JNI. Internal failures include a diagnostic report. */
  @Keep
  @SuppressWarnings("unused")
  @MainThread
  void onRoutingEvent(int resultCode, @NonNull String[] missingMaps, @Nullable String report);
}
