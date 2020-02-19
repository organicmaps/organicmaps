package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import com.github.mikephil.charting.formatter.DefaultValueFormatter;

public class AxisValueFormatter extends DefaultValueFormatter
{
  private static final String DEF_DIMEN = "m";
  private static final int DEF_DIGITS = 1;

  @NonNull
  private String mDimen = DEF_DIMEN;

  public AxisValueFormatter()
  {
    super(DEF_DIGITS);
  }

  @Override
  public String getFormattedValue(float value)
  {
    return super.getFormattedValue(value) + mDimen;
  }

  public void setDimen(@NonNull String dimen)
  {
    mDimen = dimen;
  }
}
