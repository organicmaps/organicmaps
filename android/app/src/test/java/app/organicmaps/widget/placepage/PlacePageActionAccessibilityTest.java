package app.organicmaps.widget.placepage;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.Activity;
import android.app.Application;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, qualifiers = "ru")
public class PlacePageActionAccessibilityTest
{
  @Test
  public void everyActionHasAButtonRoleAndRetainsItsClick() throws Exception
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    activity.setTheme(R.style.MwmTheme);
    PlacePageButtons fragment = spy(new PlacePageButtons());
    doReturn(activity).when(fragment).requireContext();
    Field container = PlacePageButtons.class.getDeclaredField("mButtonsContainer");
    container.setAccessible(true);
    LinearLayout host = new LinearLayout(activity);
    activity.setContentView(host);
    container.set(fragment, host);
    AtomicReference<PlacePageButtons.ButtonType> clicked = new AtomicReference<>();
    Field listener = PlacePageButtons.class.getDeclaredField("mItemListener");
    listener.setAccessible(true);
    listener.set(fragment, (PlacePageButtons.PlacePageButtonClickListener) clicked::set);
    Method create = PlacePageButtons.class.getDeclaredMethod("createButton", PlacePageButton.class, boolean.class);
    create.setAccessible(true);
    for (var type : PlacePageButtons.ButtonType.values())
    {
      View view = (View) create.invoke(fragment, new PlacePageButton(R.string.save, R.drawable.ic_share, type), false);
      host.addView(view);
      var node = AccessibilityNodeInfoCompat.wrap(view.createAccessibilityNodeInfo());
      assertEquals(Button.class.getName(), node.getClassName().toString());
      assertEquals(activity.getString(R.string.save), node.getContentDescription().toString());
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, view.findViewById(R.id.title).getImportantForAccessibility());
      assertTrue(node.isClickable());
      assertTrue(view.isFocusable());
      assertNull(view.findViewById(R.id.icon).getContentDescription());
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, view.findViewById(R.id.icon).getImportantForAccessibility());
      assertEquals(activity.getString(R.string.save), ((TextView) view.findViewById(R.id.title)).getText().toString());
      if (type != PlacePageButtons.ButtonType.MORE)
      {
        assertTrue(view.performClick());
        assertEquals(type, clicked.get());
      }
      host.removeView(view);
    }
  }
}
