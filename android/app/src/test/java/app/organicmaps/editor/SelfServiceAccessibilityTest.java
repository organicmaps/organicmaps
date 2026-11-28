package app.organicmaps.editor;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Utils;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class SelfServiceAccessibilityTest
{
  @Test
  public void radioLabelAndCheckedStateFollowRebindingAndAccessibleClicks()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Context context = activity;
    Shadows
        .shadowOf(
            (android.view.accessibility.AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE))
        .setEnabled(true);
    SelfServiceFragment fragment = mock(SelfServiceFragment.class);
    SelfServiceAdapter adapter = new SelfServiceAdapter(fragment, "yes");
    LinearLayout row = new LinearLayout(context);
    TextView text = new TextView(context);
    text.setId(R.id.self_service_default);
    RadioButton radio = new RadioButton(context);
    radio.setId(R.id.self_service_selected);
    row.addView(radio);
    row.addView(text);
    activity.setContentView(row);
    SelfServiceAdapter.ViewHolder holder = adapter.new ViewHolder(row);
    try (MockedStatic<Utils> utils = mockStatic(Utils.class))
    {
      utils.when(() -> Utils.getTagValueLocalized(any(Context.class), eq("self_service"), any(String.class)))
          .thenAnswer(call -> "Self-service " + call.getArgument(2));
      holder.bind(0);
      assertEquals("Self-service yes", radio.getContentDescription());
      AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.wrap(radio.createAccessibilityNodeInfo());
      assertEquals("Self-service yes", info.getContentDescription());
      assertTrue(info.isCheckable());
      assertTrue(info.isChecked());
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, row.getImportantForAccessibility());
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, text.getImportantForAccessibility());
      assertFalse(row.isFocusable());
      holder.bind(3);
      info = AccessibilityNodeInfoCompat.wrap(radio.createAccessibilityNodeInfo());
      assertEquals("Self-service no", info.getContentDescription());
      assertFalse(info.isChecked());
      assertTrue(radio.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
      assertEquals("no", adapter.getSelected());
      verify(fragment).saveSelection("no");
      holder.bind(3);
      assertTrue(radio.isChecked());
      assertTrue(radio.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
      assertEquals("no", adapter.getSelected());
      verify(fragment, times(2)).saveSelection("no");
      holder.bind(1);
      row.performClick();
      assertEquals("only", adapter.getSelected());
      verify(fragment).saveSelection("only");
    }
  }
}
