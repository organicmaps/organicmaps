package app.organicmaps.util;

import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import app.organicmaps.R;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class SpeedFormatted
{
  /**
   * IMPORTANT : Order of enum values MUST BE the same as native measurement_utils::Units enum.
   */
  public enum Units
  {
    KilometersPerHour(R.string.kilometers_per_hour),
    MilesPerHour(R.string.miles_per_hour);

    @StringRes
    public final int mStringRes;

    Units(@StringRes int stringRes)
    {
      mStringRes = stringRes;
    }
  }

  public final double mSpeed;
  @NonNull
  public final String mSpeedStr;
  public final SpeedFormatted.Units mUnits;

  public SpeedFormatted(double mSpeed, @NonNull String mSpeedStr, byte unitsIndex)
  {
    this.mSpeed = mSpeed;
    this.mSpeedStr = mSpeedStr;
    this.mUnits = Units.values()[unitsIndex];
  }

  public boolean isValid()
  {
    return mSpeed >= 0.0;
  }

  @NonNull
  public String getUnitsStr(@NonNull final Context context)
  {
    return context.getString(mUnits.mStringRes);
  }
}
