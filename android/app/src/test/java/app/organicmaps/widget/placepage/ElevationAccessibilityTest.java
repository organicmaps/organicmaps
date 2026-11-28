package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.SeekBar;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.StringUtils;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = {23, 35}, application = Application.class, shadows = ElevationAccessibilityTest.Formatting.class)
public class ElevationAccessibilityTest
{
  private ElevationProfileChart mChart;
  private final List<Float> mSelected = new ArrayList<>();
  private final List<Float> mFinished = new ArrayList<>();

  @Before
  public void setUp()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id)
          {
            if (id == R.string.elevation_profile_selection)
              return "Point on elevation profile";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... args)
          {
            if (id == R.string.elevation_profile_selected_distance)
              return String.format("%1$s of %2$s", args);
            if (id == R.string.elevation_profile_no_selection)
              return String.format("No point selected. Length: %s", args);
            return super.getString(id, args);
          }
        };
    Context context = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    mChart = new ElevationProfileChart(context);
    mChart.setData(new LineData(new LineDataSet(List.of(new Entry(0, 0), new Entry(100, 200)), "profile")));
    activity.setContentView(mChart);
    mChart.measure(View.MeasureSpec.makeMeasureSpec(400, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(200, View.MeasureSpec.EXACTLY));
    mChart.layout(0, 0, 400, 200);
    mChart.setOnChartValueSelectedListener(new OnChartValueSelectedListener() {
      @Override
      public void onValueSelected(Entry entry, Highlight highlight)
      {
        mSelected.add(highlight.getX());
      }
      @Override
      public void onNothingSelected()
      {}
    });
    mChart.setOnSelectionFinishedListener(mFinished::add);
  }

  @Test
  public void exposesRangeAndCurrentDistance()
  {
    mChart.highlightValue(new Highlight(25f, 50f, 0), false);
    AccessibilityNodeInfo info = mChart.createAccessibilityNodeInfo();
    assertEquals(SeekBar.class.getName(), info.getClassName());
    assertEquals(0f, info.getRangeInfo().getMin(), 0f);
    assertEquals(100f, info.getRangeInfo().getMax(), 0f);
    assertEquals(25f, info.getRangeInfo().getCurrent(), 0f);
    assertTrue(AccessibilityNodeInfoCompat.wrap(info).getStateDescription().toString().contains("25"));
  }

  @Test
  public void eachActionSelectsAndCommitsOnce()
  {
    assertTrue(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    assertEquals(List.of(5f), mSelected);
    assertEquals(List.of(5f), mFinished);
    assertEquals(10f, mChart.getHighlighted()[0].getY(), 0f);
  }

  @Test
  public void setProgressClampsToEndpointsAndRejectsInvalidArguments()
  {
    Bundle args = new Bundle();
    int action = AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SET_PROGRESS.getId();
    args.putFloat(AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE, 1000);
    assertTrue(mChart.performAccessibilityAction(action, args));
    assertEquals(100f, mChart.getHighlightX(), 0f);
    assertFalse(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    args.putFloat(AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE, -1);
    assertTrue(mChart.performAccessibilityAction(action, args));
    assertEquals(0f, mChart.getHighlightX(), 0f);
    assertFalse(mChart.performAccessibilityAction(action, null));
    args.putFloat(AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE, Float.NaN);
    assertFalse(mChart.performAccessibilityAction(action, args));
    assertEquals(List.of(100f, 0f), mFinished);
  }

  @Test
  public void keyboardReachesBothEnds()
  {
    assertTrue(
        mChart.onKeyDown(KeyEvent.KEYCODE_MOVE_END, new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MOVE_END)));
    assertEquals(100f, mChart.getHighlightX(), 0f);
    assertTrue(
        mChart.onKeyDown(KeyEvent.KEYCODE_MOVE_HOME, new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MOVE_HOME)));
    assertEquals(0f, mChart.getHighlightX(), 0f);
  }

  @Test
  public void readOnlyDisabledEmptyAndZeroLengthChartsCannotAdjust()
  {
    mChart.setTouchEnabled(false);
    assertFalse(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    mChart.setTouchEnabled(true);
    mChart.setEnabled(false);
    assertFalse(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    mChart.setEnabled(true);
    mChart.clear();
    assertFalse(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    mChart.setData(new LineData(new LineDataSet(List.of(new Entry(0, 0), new Entry(0, 10)), "profile")));
    assertFalse(mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    assertTrue(mFinished.isEmpty());
  }

  @Test
  public void reportsTheFinalSelectionWhenListenerRestoresTheMarker()
  {
    mChart.highlightValue(new Highlight(25f, 50f, 0), false);
    mChart.setOnChartValueSelectedListener(new OnChartValueSelectedListener() {
      @Override
      public void onValueSelected(Entry entry, Highlight highlight)
      {
        mChart.highlightValue(new Highlight(25f, 50f, 0), false);
      }
      @Override
      public void onNothingSelected()
      {}
    });
    mChart.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null);
    assertTrue(mFinished.isEmpty());
    assertEquals(25f, mChart.getHighlightX(), 0f);
  }
  @Test
  public void cancelledTouchStillRestoresThePreviousSelection()
  {
    mChart.highlightValue(new Highlight(25f, 50f, 0), false);
    touch(MotionEvent.ACTION_DOWN, 100);
    touch(MotionEvent.ACTION_MOVE, 300);
    touch(MotionEvent.ACTION_CANCEL, 300);
    assertEquals(25f, mChart.getHighlightX(), 0f);
    assertTrue(mFinished.isEmpty());
  }

  @Test
  public void releasedTouchStillCommitsTheFinishOnce()
  {
    mChart.highlightValue(new Highlight(25f, 50f, 0), false);
    touch(MotionEvent.ACTION_DOWN, 100);
    touch(MotionEvent.ACTION_MOVE, 300);
    touch(MotionEvent.ACTION_UP, 400);
    assertEquals(List.of(100f), mFinished);
  }

  private void touch(int action, float x)
  {
    MotionEvent event = MotionEvent.obtain(0, 100, action, x, 100, 0);
    mChart.onTouchEvent(event);
    event.recycle();
  }

  @Implements(StringUtils.class)
  public static class Formatting
  {
    @Implementation
    public static Distance nativeFormatDistance(double meters)
    {
      return new Distance(meters, Double.toString(meters), (byte) 0);
    }
  }
}
