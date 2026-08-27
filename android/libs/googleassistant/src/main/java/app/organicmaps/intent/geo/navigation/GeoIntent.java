package app.organicmaps.intent.geo.navigation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

/// Defines the user intent. If this parameter isn't set, then the default user intent is considered as `Navigation`.
///
/// <b>Example:</b><p/>
/// `geo:47.61594547836694,-122.20373173098756?q=575+Bellevue+Square,+Bellevue,+WA+98004&intent=add_a_stop`<p/>
///
/// <b>Interpretation:</b><p/>
/// The user wants to add a stop to Bellevue Square, Bellevue, with the current coordinates `[47.6, -122.2]`.
public enum GeoIntent
{
  /// Replaces the destination and starts navigation. Use this for queries like navigate to x
  Navigation("navigation"),
  /// Adds the stop as the next destination along with previous destinations. Use this for queries like add a stop at x.
  AddStop("add_a_stop"),
  /// Shows route directions without starting navigation. Use this for queries like directions to x.
  Directions("directions");

  private static final String TAG = GeoIntent.class.getSimpleName();
  @NonNull
  private final String mRaw;

  GeoIntent(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "intent";
  }

  @Nullable
  public static GeoIntent fromRaw(@Nullable String raw)
  {
    for (GeoIntent intent : values())
    {
      if (intent.mRaw.equals(raw))
        return intent;
    }
    Logger.w(TAG, "Unknown geo intent: " + raw);
    return null;
  }
}
