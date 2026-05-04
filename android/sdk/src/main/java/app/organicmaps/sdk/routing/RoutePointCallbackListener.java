package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public interface RoutePointCallbackListener {
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  void onRoutePointCallback(@NonNull String callback);
}
