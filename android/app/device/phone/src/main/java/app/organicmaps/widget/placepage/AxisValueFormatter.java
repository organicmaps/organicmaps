package app.organicmaps.widget.placepage;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.StringUtils;
import com.github.mikephil.charting.charts.BarLineChartBase;
import com.github.mikephil.charting.formatter.DefaultValueFormatter;

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
    return StringUtils.nativeFormatDistance(value).toString(mChart.getContext());
  }
}
