package app.organicmaps.widget.placepage;

import androidx.annotation.NonNull;

import com.github.mikephil.charting.charts.BarLineChartBase;
import com.github.mikephil.charting.formatter.DefaultValueFormatter;
import app.organicmaps.Framework;
import app.organicmaps.util.StringUtils;

public class AxisValueFormatter extends DefaultValueFormatter
{
  private static final int DEF_DIGITS = 1;
  private static final int ONE_KM = 1000;
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
    if (mChart.getVisibleXRange() <= ONE_KM)
      return Framework.nativeFormatAltitude(value);

    return StringUtils.nativeFormatDistance(value);
  }
}
