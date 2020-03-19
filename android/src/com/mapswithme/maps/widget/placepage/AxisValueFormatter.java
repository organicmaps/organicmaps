package com.mapswithme.maps.widget.placepage;

import com.github.mikephil.charting.formatter.DefaultValueFormatter;
import com.mapswithme.util.StringUtils;

public class AxisValueFormatter extends DefaultValueFormatter
{
  private static final int DEF_DIGITS = 1;


  public AxisValueFormatter()
  {
    super(DEF_DIGITS);
  }

  @Override
  public String getFormattedValue(float value)
  {
    return StringUtils.nativeFormatDistance(value);
  }
}
