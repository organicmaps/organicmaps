package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.NonNull;

public enum RoadWarningMarkType implements MapObjectData
{
  // Values must match the native RoadWarningMarkType enum (libs/map/routing_mark.hpp).
  TOLL(0),
  FERRY(1),
  DIRTY(2),
  STEPS(3),
  GATE(4),
  LIFT_GATE(5);

  private final int mValue;

  RoadWarningMarkType(int value)
  {
    mValue = value;
  }

  @NonNull
  public static RoadWarningMarkType fromInt(int value)
  {
    for (final RoadWarningMarkType mode : values())
    {
      if (mode.mValue == value)
        return mode;
    }
    throw new IllegalArgumentException("Invalid RoadWarningMarkType value: " + value);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeInt(mValue);
  }

  public static final Creator<RoadWarningMarkType> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public RoadWarningMarkType createFromParcel(@NonNull Parcel source)
    {
      return fromInt(source.readInt());
    }

    @Override
    @NonNull
    public RoadWarningMarkType[] newArray(int size)
    {
      return new RoadWarningMarkType[size];
    }
  };
}
