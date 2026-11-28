package app.organicmaps.widget.placepage.sections;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mockStatic;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.util.Utils;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class PlacePhoneAccessibilityTest
{
  @Test
  public void namedActionsCallAndCopyTheReboundPhoneNumber()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id)
          {
            if (id == R.string.place_phone_call)
              return "Call";
            if (id == R.string.place_phone_copy)
              return "Copy phone number";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... args)
          {
            if (id == R.string.copied_to_clipboard)
              return "Copied " + args[0];
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
    LinearLayout row = new LinearLayout(context);
    TextView phone = new TextView(context);
    phone.setId(R.id.tv__place_phone);
    row.addView(phone);
    activity.setContentView(row);
    PlacePhoneAdapter.ViewHolder holder = new PlacePhoneAdapter.ViewHolder(row);
    holder.setPhone("+1 111");
    try (MockedStatic<Utils> utils = mockStatic(Utils.class))
    {
      AccessibilityNodeInfoCompat node = AccessibilityNodeInfoCompat.wrap(row.createAccessibilityNodeInfo());
      assertTrue(node.getActionList().stream().anyMatch(action
                                                        -> action.getId() == AccessibilityNodeInfoCompat.ACTION_CLICK
                                                               && "Call".contentEquals(action.getLabel())));
      assertTrue(
          node.getActionList().stream().anyMatch(action
                                                 -> action.getId() == AccessibilityNodeInfoCompat.ACTION_LONG_CLICK
                                                        && "Copy phone number".contentEquals(action.getLabel())));
      assertTrue(row.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
      utils.verify(() -> Utils.callPhone(eq(context), eq("+1 111")));
      holder.setPhone("+2 222");
      assertTrue(row.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_LONG_CLICK, null));
      utils.verify(() -> Utils.copyTextToClipboard(eq(context), eq("+2 222")));
      assertEquals("+2 222", phone.getText().toString());
    }
  }
}
