package app.organicmaps.intent.geo.navigation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

/// Travel mode represents the method of transportation specified in the query by the user.
///
/// <b>Example:</b><p/>
/// `geo:0,0?q=Googleplex&mode=r`<p/>
///
/// <b>Interpretation:</b><p/>
/// The user wants to navigate to Googleplex using public transit.
public enum TravelMode
{
  Bicycle('b'),
  Drive('d'),
  Taxi('x'),
  TwoWheeler('l'),
  Transit('r'),
  Walk('w');

  private static final String TAG = TravelMode.class.getSimpleName();
  private final char mRaw;

  TravelMode(char raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "mode";
  }

  @Nullable
  public static TravelMode fromRaw(char raw)
  {
    for (TravelMode mode : values())
    {
      if (mode.mRaw == raw)
        return mode;
    }
    Logger.w(TAG, "Unknown travel mode: " + raw);
    return null;
  }

  @Nullable
  public static TravelMode fromRaw(@Nullable String raw)
  {
    if (raw == null || raw.length() != 1)
      return null;

    return fromRaw(raw.charAt(0));
  }
}
