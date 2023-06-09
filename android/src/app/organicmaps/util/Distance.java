package app.organicmaps.util;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import app.organicmaps.R;

import java.util.Objects;

public final class Distance
{
  /**
   * IMPORTANT : Order of enum values MUST BE the same
   * with native LaneWay enum (see platform/distance.hpp for details).
   */
  public enum Units
  {
    Meters(R.string.meter),
    Kilometers(R.string.kilometer),
    Feet(R.string.foot),
    Miles(R.string.mile);

    @StringRes
    public final int mStringRes;

    Units(@StringRes int stringRes)
    {
      mStringRes = stringRes;
    }
  }

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
    final double EPS = 1e-5;
    return mDistance > EPS;
  }

  @NonNull
  public String getUnitsStr(@NonNull final Context context)
  {
    Objects.requireNonNull(context);
    return context.getString(mUnits.mStringRes);
  }

  @NonNull
  public String toString(@NonNull final Context context)
  {
    final char nonBreakingSpace = '\u00A0';
    return mDistanceStr + nonBreakingSpace + getUnitsStr(context);
  }
}
