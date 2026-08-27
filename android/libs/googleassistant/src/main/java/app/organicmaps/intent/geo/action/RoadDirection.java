package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum RoadDirection
{
  ThisSide("this_side"),
  OtherSide("other_side");

  private static final String TAG = RoadDirection.class.getSimpleName();

  @NonNull
  private final String mRaw;

  RoadDirection(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "road_direction";
  }

  @Nullable
  public static RoadDirection fromRaw(@Nullable String raw)
  {
    for (RoadDirection direction : values())
    {
      if (direction.mRaw.equals(raw))
        return direction;
    }
    Logger.w(TAG, "Unknown road direction: " + raw);
    return null;
  }
}
