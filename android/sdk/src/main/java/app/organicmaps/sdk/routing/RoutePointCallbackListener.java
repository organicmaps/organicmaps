package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public interface RoutePointCallbackListener
{
  void onRoutePointCallback(@NonNull String callback);
}
