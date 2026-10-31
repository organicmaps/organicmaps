package app.organicmaps.sdk.widgets.speedlimit;

import static org.junit.Assert.assertEquals;

import android.content.pm.ApplicationInfo;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public class CombinedSpeedViewLayoutTest
{
  @Before
  public void enableRtl()
  {
    RuntimeEnvironment.getApplication().getApplicationInfo().flags |= ApplicationInfo.FLAG_SUPPORTS_RTL;
  }

  @Test
  public void horizontalPositionsRespectLayoutDirection()
  {
    for (int direction : new int[] {View.LAYOUT_DIRECTION_LTR, View.LAYOUT_DIRECTION_RTL})
    {
      assertCurrentSpeedLeft("left", direction, 0);
      assertCurrentSpeedLeft("right", direction, 60);
      assertCurrentSpeedLeft("start", direction, direction == View.LAYOUT_DIRECTION_RTL ? 60 : 0);
      assertCurrentSpeedLeft("end", direction, direction == View.LAYOUT_DIRECTION_RTL ? 0 : 60);
    }
  }

  @Test
  public void relativePositionsFollowDirectionChanges()
  {
    for (String position : new String[] {"start", "end"})
    {
      CombinedSpeedView view = composite(position);
      view.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
      layout(view, 120, 60);
      assertEquals(position.equals("start") ? 0 : 60, view.getChildAt(0).getLeft());

      view.setLayoutDirection(View.LAYOUT_DIRECTION_RTL);
      layout(view, 120, 60);
      assertEquals(position.equals("start") ? 60 : 0, view.getChildAt(0).getLeft());
    }
  }

  @Test
  public void endAnchoredCurrentSpeedStaysPutWhenLimitChanges()
  {
    for (int direction : new int[] {View.LAYOUT_DIRECTION_LTR, View.LAYOUT_DIRECTION_RTL})
    {
      for (int height : new int[] {60, 68})
      {
        FrameLayout parent = new FrameLayout(RuntimeEnvironment.getApplication());
        parent.setLayoutDirection(direction);
        CombinedSpeedView view = composite("end");
        parent.addView(view, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, height, Gravity.END));
        View current = view.getChildAt(0);
        int expectedLeft = direction == View.LAYOUT_DIRECTION_RTL ? 0 : 400 - height;

        for (Integer limit : new Integer[] {null, 90, null, 120, null})
        {
          view.setSpeedLimit(limit);
          layout(parent, 400, height);
          assertEquals(direction, view.getLayoutDirection());
          assertEquals(limit == null ? height : height * 2, view.getWidth());
          assertEquals(expectedLeft, view.getLeft() + current.getLeft());
          assertEquals(expectedLeft + height, view.getLeft() + current.getRight());
        }
      }
    }
  }

  private static void assertCurrentSpeedLeft(String position, int direction, int expectedLeft)
  {
    CombinedSpeedView view = composite(position);
    view.setLayoutDirection(direction);
    layout(view, 120, 60);
    assertEquals(expectedLeft, view.getChildAt(0).getLeft());
    assertEquals(60 - expectedLeft, view.getChildAt(1).getLeft());
  }

  @NonNull
  private static CombinedSpeedView composite(String position)
  {
    CombinedSpeedView view = new CombinedSpeedView(
        RuntimeEnvironment.getApplication(),
        Robolectric.buildAttributeSet().addAttribute(R.attr.om_widget_current_speed_position, position).build());
    view.setSpeedLimit(90);
    return view;
  }

  private static void layout(@NonNull View view, int width, int height)
  {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
  }
}
