package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum RoadClosureType
{
  Full("full"),
  Partial("partial");

  private static final String TAG = RoadClosureType.class.getSimpleName();

  @NonNull
  private final String mRaw;

  RoadClosureType(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "road_closure_type";
  }

  @Nullable
  public static RoadClosureType fromRaw(@Nullable String raw)
  {
    for (RoadClosureType type : values())
    {
      if (type.mRaw.equals(raw))
        return type;
    }
    Logger.w(TAG, "Unknown road closure type: " + raw);
    return null;
  }
}
