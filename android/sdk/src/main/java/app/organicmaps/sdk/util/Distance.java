package app.organicmaps.sdk.util;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import app.organicmaps.sdk.R;
import java.util.Objects;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public final class Distance implements Parcelable
{
  public static final Distance EMPTY = new Distance(0.0, "", (byte) 0);

  /**
   * IMPORTANT : Order of enum values MUST BE the same
   * with native Distance::Units enum (see platform/distance.hpp for details).
   */
  public enum Units
  {
    Meters(R.string.m),
    Kilometers(R.string.km),
    Feet(R.string.ft),
    Miles(R.string.mi);

    @StringRes
    public final int mStringRes;

    Units(@StringRes int stringRes)
    {
      mStringRes = stringRes;
    }
  }

  /// @todo What is the difference with cpp: kNarrowNonBreakingSpace = "\u202F" ?
  private static final char NON_BREAKING_SPACE = '\u00A0';

  public final double mDistance;
  @NonNull
  public final String mDistanceStr;
  public final Units mUnits;

  public Distance(double distance, @NonNull String distanceStr, byte unitsIndex)
  {
    mDistance = distance;
    mDistanceStr = distanceStr;
    mUnits = Units.values()[unitsIndex];
  }

  public boolean isValid()
  {
    return mDistance >= 0.0;
  }

  @NonNull
  public String getUnitsStr(@NonNull final Context context)
  {
    return context.getString(mUnits.mStringRes);
  }

  @NonNull
  public String toString(@NonNull final Context context)
  {
    if (!isValid())
      return "";

    return mDistanceStr + NON_BREAKING_SPACE + getUnitsStr(context);
  }

  @NonNull
  @Override
  public String toString()
  {
    if (!isValid())
      return "";

    return mDistanceStr + NON_BREAKING_SPACE + mUnits.toString();
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    final Distance distance = (Distance) o;
    return Double.compare(distance.mDistance, mDistance) == 0 && mDistanceStr.equals(distance.mDistanceStr)
 && mUnits == distance.mUnits;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeDouble(mDistance);
    dest.writeString(mDistanceStr);
    dest.writeByte((byte) mUnits.ordinal());
  }

  public static final Creator<Distance> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public Distance createFromParcel(@NonNull Parcel source)
    {
      return new Distance(source.readDouble(),
                          Objects.requireNonNull(source.readString(), "Distance string cannot be null"),
                          source.readByte());
    }

    @Override
    @NonNull
    public Distance[] newArray(int size)
    {
      return new Distance[size];
    }
  };
}
