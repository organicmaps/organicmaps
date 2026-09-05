package app.organicmaps.intent.geo;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.intent.IntentUtils;
import app.organicmaps.sdk.util.Assert;
import java.util.Map;
import java.util.Set;

/// Use a search intent to search for a query and display multiple results along the route while driving.
/// @see <a
/// href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app#search-intent">Intent
/// format</a>
public class GeoViewIntent
{
  private static final Set<String> sSupportedSchemes = Set.of("geo", "geo.offline");
  private static final String QUERY_KEY = "q";

  private final double mLat;
  private final double mLon;
  @Nullable
  private final String mQuery;

  private GeoViewIntent(double lat, double lon, @Nullable String query)
  {
    mLat = lat;
    mLon = lon;
    mQuery = query;
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  @Nullable
  public String getQuery()
  {
    return mQuery;
  }

  public static boolean isSearchIntent(@NonNull final Intent intent)
  {
    if (!Intent.ACTION_VIEW.equals(intent.getAction()))
      return false;

    return sSupportedSchemes.contains(intent.getScheme());
  }

  @NonNull
  public static GeoViewIntent fromIntent(@NonNull final Intent intent)
  {
    Assert.debug(isSearchIntent(intent), "Not a navigation intent");

    final double[] latLon = IntentUtils.getCoordinates(intent);
    final Map<String, String> queryParameters = IntentUtils.getQueryParameters(intent);
    return new GeoViewIntent(latLon[0], latLon[1], queryParameters.get(QUERY_KEY));
  }
}
