package app.organicmaps.widget.placepage;

import androidx.annotation.NonNull;

import com.github.mikephil.charting.charts.BarLineChartBase;
import com.github.mikephil.charting.formatter.DefaultValueFormatter;
import app.organicmaps.sdk.Framework;

public class AxisValueFormatter extends DefaultValueFormatter
{
  private static final int DEF_DIGITS = 1;
  @NonNull
  private final BarLineChartBase mChart;

  public AxisValueFormatter(@NonNull BarLineChartBase chart)
  {
    super(DEF_DIGITS);
    mChart = chart;
  }

  @Override
  public String getFormattedValue(float value)
  {
    return Framework.nativeFormatAltitude(value);
  }
}
