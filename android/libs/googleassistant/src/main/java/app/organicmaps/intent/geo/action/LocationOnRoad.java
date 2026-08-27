package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum LocationOnRoad
{
  OnRoad("on_road"),
  OnShoulder("on_shoulder");

  private static final String TAG = LocationOnRoad.class.getSimpleName();

  @NonNull
  private final String mRaw;

  LocationOnRoad(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "location_on_road";
  }

  @Nullable
  public static LocationOnRoad fromRaw(@Nullable String raw)
  {
    for (LocationOnRoad location : values())
    {
      if (location.mRaw.equals(raw))
        return location;
    }
    Logger.w(TAG, "Unknown location on road: " + raw);
    return null;
  }
}
