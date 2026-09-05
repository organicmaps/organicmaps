package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum HazardType
{
  Animal("animal"),
  BrokenTrafficLight("broken_traffic_light"),
  Construction("construction"),
  Flooding("flooding"),
  Fog("fog"),
  Hail("hail"),
  Ice("ice"),
  MissingSign("missing_sign"),
  ObjectOnRoad("object_on_road"),
  Pothole("pothole"),
  Roadkill("roadkill"),
  Snow("snow"),
  Vehicle("vehicle"),
  Weather("weather");

  private static final String TAG = HazardType.class.getSimpleName();

  @NonNull
  private final String mRaw;

  HazardType(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "hazard_type";
  }

  @Nullable
  public static HazardType fromRaw(@Nullable String raw)
  {
    for (HazardType type : values())
    {
      if (type.mRaw.equals(raw))
        return type;
    }
    Logger.w(TAG, "Unknown hazard type: " + raw);
    return null;
  }
}
