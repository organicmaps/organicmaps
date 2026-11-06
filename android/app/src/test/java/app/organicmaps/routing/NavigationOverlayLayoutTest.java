package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.core.graphics.Insets;
import app.organicmaps.R;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedView;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 35, application = Application.class)
public class NavigationOverlayLayoutTest
{
  @Test
  public void sideInsetsProtectSignsAndLanesWithoutAccumulating()
  {
    for (boolean landscape : new boolean[] {false, true})
    {
      for (int direction : new int[] {View.LAYOUT_DIRECTION_LTR, View.LAYOUT_DIRECTION_RTL})
      {
        Context context = context(landscape);
        ViewGroup parent = (ViewGroup) LayoutInflater.from(context).inflate(R.layout.layout_nav_top, null);
        parent.setLayoutDirection(direction);
        View lanes = parent.findViewById(R.id.lanes);
        lanes.setVisibility(View.VISIBLE);
        CombinedSpeedView speed = parent.findViewById(R.id.nav_speed_limit);
        Insets insets = direction == View.LAYOUT_DIRECTION_RTL ? Insets.of(48, 0, 0, 0) : Insets.of(0, 0, 48, 0);

        for (Integer limit : new Integer[] {90, null, 50})
        {
          speed.setSpeedLimit(limit);
          NavigationController.applySideInsets(parent, insets);
          NavigationController.applySideInsets(parent, insets);
          layout(parent, 800, 360);
          assertEquals(56, ((ViewGroup.MarginLayoutParams) speed.getLayoutParams()).getMarginEnd());
          for (View view : new View[] {speed, lanes})
          {
            if (direction == View.LAYOUT_DIRECTION_RTL)
              assertTrue(view.getLeft() >= 56);
            else
              assertTrue(view.getRight() <= 744);
          }
        }
      }
    }
  }

  private static Context context(boolean landscape)
  {
    RuntimeEnvironment.setQualifiers(landscape ? "w800dp-h360dp-land-mdpi" : "w360dp-h800dp-port-mdpi");
    Application application = RuntimeEnvironment.getApplication();
    application.getApplicationInfo().flags |= ApplicationInfo.FLAG_SUPPORTS_RTL;
    return new ContextThemeWrapper(application, R.style.MwmTheme);
  }

  private static void layout(View view, int width, int height)
  {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
  }
}
