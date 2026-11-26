package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.routing.TransitStepType;
import app.organicmaps.util.ThemeUtils;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.Locale;
import kotlin.jvm.functions.Function1;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class TransitAccessibilityTest
{
  private Context context(Activity activity)
  {
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public int getDimensionPixelSize(int id)
          {
            return id >>> 24 == 0x7f ? 8 : super.getDimensionPixelSize(id);
          }
          @Override
          public float getDimension(int id)
          {
            return id >>> 24 == 0x7f ? 8f : super.getDimension(id);
          }
          @Override
          public String getString(int id)
          {
            if (id == R.string.editor_section_expanded)
              return "Expanded";
            if (id == R.string.editor_section_collapsed)
              return "Collapsed";
            if (id == R.string.routing_options_walking)
              return "Walking";
            if (id == R.string.transit_waypoint_label)
              return "Stop %1$d";
            if (id == R.string.transit_type_bus)
              return "Bus";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... arguments)
          {
            return String.format(Locale.US, getString(id), arguments);
          }
          @Override
          public String getQuantityString(int id, int quantity, Object... arguments)
          {
            if (id == R.plurals.transit_stops_count)
              return quantity + " stops";
            return super.getQuantityString(id, quantity, arguments);
          }
        };
    return new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
  }

  private static TransitStepInfo step(TransitStepType type, String... stops) throws Exception
  {
    Constructor<?> constructor = TransitStepInfo.class.getDeclaredConstructors()[0];
    constructor.setAccessible(true);
    return (TransitStepInfo) constructor.newInstance(type.ordinal(), "200", "m", 180, "41", 0xff0060a8, 2, "Start",
                                                     "End", stops.length + 1, stops, "41");
  }

  @Test
  public void stepDescriptionsKeepTheirMeaningWhenTheViewIsRebound() throws Exception
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().visible().get();
    Context context = context(activity);
    try (MockedStatic<ThemeUtils> ignored = Mockito.mockStatic(ThemeUtils.class))
    {
      TransitStepView view = new TransitStepView(context, null);
      view.setTransitStepInfo(step(TransitStepType.PEDESTRIAN));
      assertEquals("Walking", view.getContentDescription());
      view.setTransitStepInfo(TransitStepInfo.intermediatePoint(2));
      assertEquals("Stop 3", view.getContentDescription());
      view.setTransitStepInfo(step(TransitStepType.BUS));
      assertEquals("Bus 41", view.getContentDescription());
      view.setTransitStepInfo(TransitStepInfo.ruler("1.2", "km"));
      assertEquals("1.2 km", view.getContentDescription());
    }
  }

  private static void add(LinearLayout parent, View child, int id)
  {
    child.setId(id);
    parent.addView(
        child, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
  }

  @Test
  public void disclosureActionsFollowStateAndDisappearOnRebindWithoutStops() throws Exception
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().visible().get();
    Context context = context(activity);
    try (MockedStatic<ThemeUtils> ignored = Mockito.mockStatic(ThemeUtils.class))
    {
      LinearLayout root = new LinearLayout(context);
      root.setOrientation(LinearLayout.VERTICAL);
      for (int id : new int[] {R.id.board_at, R.id.exit_at, R.id.intermediate_stops, R.id.transfer_header})
        add(root, new TextView(context), id);
      for (int id : new int[] {R.id.line_bar, R.id.transfer_header_row, R.id.transfer_stub_top,
                               R.id.transfer_stub_bottom, R.id.transfer_gap})
        add(root, new View(context), id);
      LinearLayout summary = new LinearLayout(context);
      add(root, summary, R.id.ride_summary_row);
      add(summary, new TransitStepView(context, null), R.id.line_badge);
      add(summary, new TextView(context), R.id.ride_subtitle);
      activity.setContentView(root);
      Class<?> type = Class.forName(TransitDetailsAdapter.class.getName() + "$RideViewHolder");
      Constructor<?> constructor = type.getDeclaredConstructor(View.class, Function1.class);
      constructor.setAccessible(true);
      Object holder = constructor.newInstance(root, (Function1<Integer, String>) value -> "3 min");
      Method bind =
          type.getDeclaredMethod("bind", TransitStepInfo.class, boolean.class, boolean.class, boolean.class, int.class);
      Method setExpanded = type.getDeclaredMethod("setExpanded", boolean.class, boolean.class);
      bind.setAccessible(true);
      setExpanded.setAccessible(true);
      bind.invoke(holder, step(TransitStepType.BUS, "Middle"), false, false, false, 0xff0060a8);
      summary.setOnClickListener(v -> {
        try
        {
          setExpanded.invoke(holder, !"Expanded".contentEquals(ViewCompat.getStateDescription(summary)), true);
        }
        catch (ReflectiveOperationException e)
        {
          throw new AssertionError(e);
        }
      });
      var node = AccessibilityNodeInfoCompat.wrap(summary.createAccessibilityNodeInfo());
      assertEquals("android.widget.Button", node.getClassName());
      assertEquals("Collapsed", ViewCompat.getStateDescription(summary));
      assertTrue(summary.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
      assertEquals("Expanded", ViewCompat.getStateDescription(summary));
      assertFalse(summary.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
      assertTrue(summary.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
      assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS,
                   root.findViewById(R.id.intermediate_stops).getImportantForAccessibility());
      bind.invoke(holder, step(TransitStepType.BUS), false, false, false, 0xff0060a8);
      assertNull(ViewCompat.getStateDescription(summary));
      assertFalse(summary.isClickable());
      assertFalse(summary.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
      assertEquals("", ((TextView) root.findViewById(R.id.intermediate_stops)).getText().toString());
    }
  }
}
