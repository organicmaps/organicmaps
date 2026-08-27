package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

public enum AccidentType
{
  Minor("minor"),
  Major("major");

  private static final String TAG = AccidentType.class.getSimpleName();

  @NonNull
  private final String mRaw;

  AccidentType(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "accident_type";
  }

  @Nullable
  public static AccidentType fromRaw(@Nullable String raw)
  {
    for (AccidentType type : values())
    {
      if (type.mRaw.equals(raw))
        return type;
    }
    Logger.w(TAG, "Unknown accident type: " + raw);
    return null;
  }
}
