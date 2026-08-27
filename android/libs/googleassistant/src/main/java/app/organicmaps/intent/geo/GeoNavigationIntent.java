package app.organicmaps.intent.geo;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.intent.IntentUtils;
import app.organicmaps.intent.geo.navigation.Avoid;
import app.organicmaps.intent.geo.navigation.GeoIntent;
import app.organicmaps.intent.geo.navigation.TravelMode;
import app.organicmaps.sdk.util.Assert;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/// Use a navigation intent to fulfill a user's request to navigate to a specific destination. This destination can
/// either be a single location (address) or multiple locations (for example, coffee shops and gas stations).
///
/// Intent data follows a URI format specified for each intent.
/// @see <a
/// href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app#intent-format">Intent
/// format</a>
public class GeoNavigationIntent
{
  private static final Set<String> sSupportedActions =
      Set.of("androidx.car.app.action.NAVIGATE", "android.intent.action.NAVIGATE");
  private static final Set<String> sSupportedSchemes = Set.of("geo", "geo.offline");
  private static final String QUERY_KEY = "q";
  private static final String ENTRY_KEY = "entry";

  private final double mLat;
  private final double mLon;
  @Nullable
  private final String mQuery;
  @NonNull
  private final GeoIntent mIntent;
  @Nullable
  private final List<Avoid> mAvoidList;
  @Nullable
  private final TravelMode mTravelMode;
  /// Used for logging the source of entry.
  ///
  /// Possible values: assistant.
  @Nullable
  private final String mEntry;

  private GeoNavigationIntent(double lat, double lon, @Nullable String query, @NonNull GeoIntent intent,
                              @Nullable List<Avoid> avoidList, @Nullable TravelMode travelMode, @Nullable String entry)
  {
    mLat = lat;
    mLon = lon;
    mQuery = query;
    mIntent = intent;
    mAvoidList = avoidList;
    mTravelMode = travelMode;
    mEntry = entry;
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

  @NonNull
  public GeoIntent getIntent()
  {
    return mIntent;
  }

  @Nullable
  public List<Avoid> getAvoidList()
  {
    return mAvoidList;
  }

  @Nullable
  public TravelMode getTravelMode()
  {
    return mTravelMode;
  }

  @Nullable
  public String getEntry()
  {
    return mEntry;
  }

  public static boolean isNavigationIntent(@NonNull final Intent intent)
  {
    if (!sSupportedActions.contains(intent.getAction()))
      return false;

    return sSupportedSchemes.contains(intent.getScheme());
  }

  @NonNull
  public static GeoNavigationIntent fromIntent(@NonNull final Intent intent)
  {
    Assert.debug(isNavigationIntent(intent), "Not a navigation intent");

    final double[] latLon = IntentUtils.getCoordinates(intent);
    final Map<String, String> queryParameters = IntentUtils.getQueryParameters(intent);
    final String query = queryParameters.get(QUERY_KEY);
    final GeoIntent geoIntent = GeoIntent.fromRaw(queryParameters.get(GeoIntent.getKey()));
    final List<Avoid> avoidList = Avoid.fromRaw(queryParameters.get(Avoid.getKey()));
    final TravelMode travelMode = TravelMode.fromRaw(queryParameters.get(TravelMode.getKey()));
    final String entry = queryParameters.get(ENTRY_KEY);
    return new GeoNavigationIntent(latLon[0], latLon[1], query,
                                   Objects.requireNonNullElse(geoIntent, GeoIntent.Navigation), avoidList, travelMode,
                                   entry);
  }
}
