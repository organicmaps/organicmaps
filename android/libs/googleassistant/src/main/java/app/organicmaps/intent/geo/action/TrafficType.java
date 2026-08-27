package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum TrafficType
{
  Moderate("moderate"),
  Heavy("heavy"),
  Standstill("standstill");

  private static final String TAG = TrafficType.class.getSimpleName();

  @NonNull
  private final String mRaw;

  TrafficType(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "traffic_type";
  }

  @Nullable
  public static TrafficType fromRaw(@Nullable String raw)
  {
    for (TrafficType type : values())
    {
      if (type.mRaw.equals(raw))
        return type;
    }
    Logger.w(TAG, "Unknown traffic type: " + raw);
    return null;
  }
}
