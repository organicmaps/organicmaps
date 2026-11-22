package app.organicmaps.api;

import android.content.Intent;
import android.net.Uri;

public final class MapRequest
{
  public static boolean isNavigationAction(String action)
  {
    return "androidx.car.app.action.NAVIGATE".equals(action) || "android.intent.action.NAVIGATE".equals(action);
  }

  /** A map launch or display handoff without a payload can still recover the previous trip. */
  public static boolean isExplicit(Intent intent)
  {
    if (intent.getBooleanExtra(Const.EXTRA_PICK_POINT, false))
      return true;
    final String action = intent.getAction();
    if (Intent.ACTION_SEND.equals(action) || Intent.ACTION_SEND_MULTIPLE.equals(action))
      return intent.hasExtra(Intent.EXTRA_STREAM);
    return intent.getData() != null && !Intent.ACTION_MAIN.equals(action);
  }

  /** Android treats geo:lat,lon?... as opaque, so its query must be read from the encoded part. */
  public static Uri parameters(Uri uri)
  {
    if (!uri.isOpaque())
      return uri;
    final String part = uri.getEncodedSchemeSpecificPart();
    final int start = part.indexOf('?');
    return new Uri.Builder().encodedQuery(start < 0 ? null : part.substring(start + 1)).build();
  }

  public static boolean addressesCurrentRoute(Intent intent)
  {
    final Uri uri = intent.getData();
    if (uri == null)
      return false;
    final String scheme = uri.getScheme();
    if (isNavigationAction(intent.getAction()) && ("geo".equals(scheme) || "geo.offline".equals(scheme)))
      return "add_a_stop".equals(parameters(uri).getQueryParameter("intent"));
    if (!"geo.action".equals(scheme) && !"geo.action.offline".equals(scheme))
      return false;
    final String action = parameters(uri).getQueryParameter("act");
    if (action == null)
      return false;
    return switch (action)
    {
      case "resume_navigation", "exit_navigation", "route_overview", "show_directions_list", "follow_mode",
          "avoid_tolls", "avoid_ferries", "avoid_highways", "allow_tolls", "allow_ferries", "allow_highways" ->
        true;
      default -> false;
    };
  }

  private MapRequest() {}
}
