package app.organicmaps.sdk.widgets.speedlimit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.view.View;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public class SpeedAccessibilityTest
{
  @Test
  public void compositeExposesOneDescriptionWithCurrentValues()
  {
    CombinedSpeedView view = new CombinedSpeedView(RuntimeEnvironment.getApplication());
    CombinedSpeedViewController controller = new CombinedSpeedViewController(view);
    assertEquals("Current speed unavailable", view.getContentDescription());
    assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_YES, view.getImportantForAccessibility());
    assertFalse(view.isClickable());
    for (int i = 0; i < view.getChildCount(); ++i)
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, view.getChildAt(i).getImportantForAccessibility());

    controller.setUnits("km/h");
    controller.setSpeed(42);
    assertEquals("Current speed: 42 km/h", view.getContentDescription());
    controller.setSpeedLimit(50);
    assertEquals("Current speed: 42 km/h, Speed limit: 50 km/h", view.getContentDescription());
    controller.setUnits("mph");
    assertEquals("Current speed: 42 mph, Speed limit: 50 mph", view.getContentDescription());
    controller.setSpeed(null);
    assertEquals("Current speed unavailable, Speed limit: 50 mph", view.getContentDescription());
    controller.setSpeed(1000);
    assertEquals("Current speed unavailable, Speed limit: 50 mph", view.getContentDescription());
    controller.hideSpeedLimit();
    assertEquals("Current speed unavailable", view.getContentDescription());
    controller.setSpeed(0);
    assertEquals("Current speed: 0 mph", view.getContentDescription());
    controller.showSpeedLimit();
    assertEquals("Current speed: 0 mph, Speed limit: 50 mph", view.getContentDescription());
    controller.setSpeedLimit(null);
    assertEquals("Current speed: 0 mph", view.getContentDescription());
  }
}
