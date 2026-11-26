package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.widget.CheckedTextView;
import android.widget.ListView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.settings.StorageItem;
import app.organicmaps.sdk.settings.StoragePathManager;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.util.ThemeUtils;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, shadows = StoragePathAccessibilityTest.ShadowFramework.class)
public class StoragePathAccessibilityTest
{
  @Test
  public void unavailableStoragesHaveNoClickActionAndAreSkippedByKeyboard()
  {
    TestActivity activity = Robolectric.buildActivity(TestActivity.class).setup().get();
    Resources original = activity.getResources();
    activity.mResources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id, Object... args)
          {
            if (id == R.string.maps_storage_free_size)
              return "Free " + args[0] + " of " + args[1];
            return super.getString(id, args);
          }
        };
    Shadows.shadowOf((AccessibilityManager) activity.getSystemService(Context.ACCESSIBILITY_SERVICE)).setEnabled(true);
    StoragePathManager manager = new StoragePathManager(activity);
    manager.mStorages.add(new StorageItem("/current", 1, 10000, "Current", false));
    manager.mStorages.add(new StorageItem("/readonly", 10000, 10000, "Read-only", true));
    manager.mStorages.add(new StorageItem("/small", 1, 10000, "Too small", false));
    manager.mStorages.add(new StorageItem("/available", 10000, 10000, "Available", false));
    manager.mCurrentStorageIndex = 0;
    StoragePathAdapter adapter = new StoragePathAdapter(manager, activity);
    adapter.update(100);
    activity.mInflater = mock(LayoutInflater.class);
    when(activity.mInflater.inflate(eq(R.layout.item_storage), any(ViewGroup.class), eq(false)))
        .thenAnswer(call -> new CheckedTextView(activity));
    try (MockedStatic<Utils> utils = mockStatic(Utils.class);
         MockedStatic<ThemeUtils> theme = mockStatic(ThemeUtils.class))
    {
      utils.when(() -> Utils.dimen(any(Context.class), anyInt())).thenReturn(12);
      ListView list = new ListView(activity);
      list.setAdapter(adapter);
      activity.setContentView(list);
      layout(list);
      assertFalse(adapter.areAllItemsEnabled());
      assertTrue(adapter.isEnabled(0));
      assertFalse(adapter.isEnabled(1));
      assertFalse(adapter.isEnabled(2));
      assertTrue(adapter.isEnabled(3));
      assertTrue(((CheckedTextView) list.getChildAt(0)).isChecked());
      for (int i : new int[] {1, 2})
      {
        AccessibilityNodeInfoCompat info =
            AccessibilityNodeInfoCompat.wrap(list.getChildAt(i).createAccessibilityNodeInfo());
        assertFalse(info.isEnabled());
        assertFalse(info.getActionList().stream().anyMatch(
            action -> action.getId() == AccessibilityNodeInfoCompat.ACTION_CLICK));
      }
      list.requestFocus();
      list.setSelection(0);
      layout(list);
      list.onKeyDown(KeyEvent.KEYCODE_DPAD_DOWN, new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_DOWN));
      assertEquals(3, list.getSelectedItemPosition());
      adapter.update(20000);
      assertFalse(adapter.isEnabled(3));
      assertTrue(adapter.isEnabled(0));
    }
  }

  private static void layout(View view)
  {
    view.measure(View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(1000, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, 600, 1000);
  }

  public static class TestActivity extends Activity
  {
    Resources mResources;
    LayoutInflater mInflater;
    @Override
    public Resources getResources()
    {
      return mResources == null ? super.getResources() : mResources;
    }
    @Override
    public LayoutInflater getLayoutInflater()
    {
      return mInflater == null ? super.getLayoutInflater() : mInflater;
    }
  }

  @Implements(Framework.class)
  public static class ShadowFramework
  {
    @Implementation
    protected static String nativeGetDataFileExt()
    {
      return ".mwm";
    }
    @Implementation
    protected static String[] nativeGetMovableFilesExts()
    {
      return new String[] {".mwm"};
    }
  }
}
