package app.organicmaps.sdk.widgets.speedlimit;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.View;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public class SpeedSignValueTest
{
  private static final int[] SAMPLES = {-1, 0, 999, 1000, Integer.MAX_VALUE, Integer.MIN_VALUE, 42};
  private static final String[] DISPLAY_VALUES = {"–", "0", "999", "–", "–", "–", "42"};

  @Test
  public void currentSpeedRepresentsOutliersAndRecovers()
  {
    CurrentSpeedView view = new CurrentSpeedView(RuntimeEnvironment.getApplication());
    layout(view);
    for (int i = 0; i < SAMPLES.length; ++i)
    {
      view.setSpeed(SAMPLES[i]);
      assertEquals(SAMPLES[i], view.getSpeed());
      assertDrawnValue(view, DISPLAY_VALUES[i]);
    }
  }

  @Test
  public void speedLimitRepresentsOutliersAndRecovers()
  {
    SpeedLimitView view = new SpeedLimitView(RuntimeEnvironment.getApplication());
    layout(view);
    for (int i = 0; i < SAMPLES.length; ++i)
    {
      view.setSpeedLimit(SAMPLES[i]);
      assertEquals(SAMPLES[i], view.getSpeedLimit());
      assertDrawnValue(view, DISPLAY_VALUES[i]);
    }
  }

  private static void assertDrawnValue(View view, String expected)
  {
    Canvas canvas = mock(Canvas.class);
    view.draw(canvas);
    verify(canvas).drawText(eq(expected), anyFloat(), anyFloat(), any(Paint.class));
  }

  private static void layout(View view)
  {
    int spec = View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY);
    view.measure(spec, spec);
    view.layout(0, 0, 100, 100);
  }
}
