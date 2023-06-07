package app.organicmaps.util;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import app.organicmaps.R;

import java.util.Objects;

public class Distance
{
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
  public final String mDistanceStr;
  public final Units mUnits;

  public Distance(double distance, String distanceStr, Units units)
  {
    mDistance = distance;
    mDistanceStr = distanceStr;
    mUnits = units;
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
    return mDistanceStr + " " + getUnitsStr(context);
  }
}
