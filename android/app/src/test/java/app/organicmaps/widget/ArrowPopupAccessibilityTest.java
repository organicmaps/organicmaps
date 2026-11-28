package app.organicmaps.widget;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.ListView;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class ArrowPopupAccessibilityTest
{
  private ListView createList(List<ArrowPopup.Item> items, ArrowPopup.RowStyle style) throws Exception
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().visible().get();
    Shadows.shadowOf((AccessibilityManager) activity.getSystemService(Context.ACCESSIBILITY_SERVICE)).setEnabled(true);
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public int getDimensionPixelSize(int id)
          {
            return id >>> 24 == 0x7f ? 8 : super.getDimensionPixelSize(id);
          }
        };
    Context context = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    Method method = ArrowPopup.class.getDeclaredMethod("createPopupList", Context.class, List.class,
                                                       ArrowPopup.RowStyle.class, float.class);
    method.setAccessible(true);
    ListView list;
    try (MockedStatic<app.organicmaps.util.ThemeUtils> ignored =
             Mockito.mockStatic(app.organicmaps.util.ThemeUtils.class))
    {
      list = (ListView) method.invoke(ArrowPopup.INSTANCE, context, items, style, 8f);
      activity.setContentView(list);
      list.measure(View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(400, View.MeasureSpec.EXACTLY));
      list.layout(0, 0, 600, 400);
    }
    return list;
  }

  @Test
  public void exposesTheCurrentTrackThroughNativeCollectionSelection() throws Exception
  {
    ListView list = createList(List.of(new ArrowPopup.Item("East", 0, false), new ArrowPopup.Item("West", 0, true)),
                               ArrowPopup.RowStyle.COLOR_ICON_LEFT);
    assertEquals(ListView.CHOICE_MODE_SINGLE, list.getChoiceMode());
    assertEquals(1, list.getCheckedItemPosition());
    var first = list.getChildAt(0).createAccessibilityNodeInfo();
    var second = list.getChildAt(1).createAccessibilityNodeInfo();
    assertEquals(android.view.accessibility.AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_SINGLE,
                 list.createAccessibilityNodeInfo().getCollectionInfo().getSelectionMode());
    assertFalse(first.getCollectionItemInfo().isSelected());
    assertTrue(second.getCollectionItemInfo().isSelected());
  }

  @Test
  public void noTrackIsCheckedWhenThereIsNoCurrentSelection() throws Exception
  {
    ListView list = createList(List.of(new ArrowPopup.Item("East", 0, false)), ArrowPopup.RowStyle.COLOR_ICON_LEFT);
    assertEquals(ListView.INVALID_POSITION, list.getCheckedItemPosition());
    assertFalse(list.getChildAt(0).createAccessibilityNodeInfo().getCollectionItemInfo().isSelected());
  }

  @Test
  public void routeActionsDoNotBecomeSingleChoiceItems() throws Exception
  {
    ListView list = createList(List.of(new ArrowPopup.Item("41", 0, false)), ArrowPopup.RowStyle.PASTEL_BACKGROUND);
    assertEquals(ListView.CHOICE_MODE_NONE, list.getChoiceMode());
    assertFalse(list.getChildAt(0).createAccessibilityNodeInfo().isCheckable());
  }
}
