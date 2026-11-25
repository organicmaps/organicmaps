package app.organicmaps.widget.colorpicker;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import android.widget.Button;
import android.widget.SeekBar;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 35, application = Application.class)
public class ColorPickerAccessibilityTest
{
  private Context mContext;
  private Activity mActivity;
  private MockedStatic<ThemeUtils> mTheme;

  @Before
  public void setUp()
  {
    mTheme = Mockito.mockStatic(ThemeUtils.class);
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    mActivity = activity;
    // Supply only presentation resources so the tests exercise control behavior independently of the app theme.
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public float getDimension(int id)
          {
            return 8f;
          }
          @Override
          public int getDimensionPixelSize(int id)
          {
            return 16;
          }
          @Override
          public String getString(int id)
          {
            if (id == R.string.color_picker_spectrum)
              return "Spectrum";
            if (id == R.string.color_picker_edit_rgb)
              return "Edit RGB values";
            if (id == R.string.color_picker_red)
              return "RED";
            if (id == R.string.color_picker_green)
              return "GREEN";
            if (id == R.string.color_picker_blue)
              return "BLUE";
            if (id == R.string.color_picker_hex_label)
              return "Color HEX #";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... args)
          {
            if (id == R.string.color_picker_rgb_value)
              return String.format("Red %1$d, green %2$d, blue %3$d", args);
            if (id == R.string.color_picker_color_description)
              return String.format("Color %1$d, %2$s", args);
            return super.getString(id, args);
          }
        };
    mContext = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
  }

  private void attach(View view)
  {
    mActivity.setContentView(view);
    view.measure(View.MeasureSpec.makeMeasureSpec(264, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(220, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, 264, 220);
  }

  @After
  public void tearDown()
  {
    mTheme.close();
  }

  @Test
  public void initialRgbValuesAndLabelsMatchTheColor()
  {
    ColorSlidersView sliders = new ColorSlidersView(mContext);
    attach(sliders);
    android.view.ViewGroup redGroup = (android.view.ViewGroup) sliders.getChildAt(0);
    android.view.ViewGroup row = (android.view.ViewGroup) redGroup.getChildAt(1);
    RoundSlider red = (RoundSlider) row.getChildAt(0);
    assertEquals(255, red.getValue());
    assertEquals(red.getId(), redGroup.getChildAt(0).getLabelFor());
    assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO, row.getChildAt(1).getImportantForAccessibility());
    android.view.ViewGroup hex = (android.view.ViewGroup) sliders.getChildAt(3);
    assertEquals(hex.getChildAt(1).getId(), hex.getChildAt(0).getLabelFor());
    assertEquals("FF0000", ((android.widget.EditText) hex.getChildAt(1)).getText().toString());
  }

  @Test
  public void sliderExposesRangeAndCurrentValue()
  {
    RoundSlider slider = new RoundSlider(mContext);
    attach(slider);
    slider.setValue(123);
    AccessibilityNodeInfo node = slider.createAccessibilityNodeInfo();
    assertEquals(SeekBar.class.getName(), node.getClassName());
    assertEquals(0f, node.getRangeInfo().getMin(), 0f);
    assertEquals(255f, node.getRangeInfo().getMax(), 0f);
    assertEquals(123f, node.getRangeInfo().getCurrent(), 0f);
    assertEquals("123", node.getStateDescription());
  }

  @Test
  @Config(sdk = {23, 35})
  public void sliderActionsClampValuesAndNotifyOnce()
  {
    RoundSlider slider = new RoundSlider(mContext);
    attach(slider);
    List<Integer> changes = new ArrayList<>();
    slider.setOnValueChangedListener((view, value) -> changes.add(value));
    slider.setValue(250);
    assertTrue(slider.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    assertEquals(255, slider.getValue());
    assertFalse(slider.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    Bundle args = new Bundle();
    args.putFloat(AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE, -30f);
    assertTrue(slider.performAccessibilityAction(
        AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SET_PROGRESS.getId(), args));
    assertEquals(0, slider.getValue());
    args.putFloat(AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE, Float.NaN);
    assertFalse(slider.performAccessibilityAction(
        AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SET_PROGRESS.getId(), args));
    assertEquals(List.of(255, 0), changes);
  }

  @Test
  public void disabledSliderCannotChange()
  {
    RoundSlider slider = new RoundSlider(mContext);
    attach(slider);
    slider.setValue(70);
    slider.setEnabled(false);
    assertFalse(slider.performAccessibilityAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD, null));
    slider.onKeyDown(KeyEvent.KEYCODE_DPAD_RIGHT, new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT));
    assertEquals(70, slider.getValue());
  }

  @Test
  public void keyboardCanReachEverySliderValue()
  {
    RoundSlider slider = new RoundSlider(mContext);
    attach(slider);
    for (int value = 1; value <= 255; ++value)
    {
      assertTrue(slider.onKeyDown(KeyEvent.KEYCODE_DPAD_RIGHT,
                                  new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT)));
      assertEquals(value, slider.getValue());
    }
    assertTrue(
        slider.onKeyDown(KeyEvent.KEYCODE_MOVE_HOME, new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MOVE_HOME)));
    assertEquals(0, slider.getValue());
  }

  @Test
  public void segmentsExposeButtonRoleAndOnlyCurrentSelection()
  {
    PillSegmentedControl control = new PillSegmentedControl(mContext);
    attach(control);
    control.setSegments("Grid", "Spectrum", "Sliders");
    assertTrue(control.getChildAt(0).createAccessibilityNodeInfo().isSelected());
    assertEquals(Button.class.getName(), control.getChildAt(1).createAccessibilityNodeInfo().getClassName());
    assertTrue(control.getChildAt(2).isFocusable());
    control.getChildAt(2).performClick();
    assertFalse(control.getChildAt(0).createAccessibilityNodeInfo().isSelected());
    assertTrue(control.getChildAt(2).createAccessibilityNodeInfo().isSelected());
  }

  @Test
  public void gridExposesAllCellsAndTheirBounds()
  {
    ColorGridView grid = new ColorGridView(mContext);
    attach(grid);
    grid.setPadding(12, 10, 12, 10);
    grid.layout(0, 0, 264, 220);
    AccessibilityNodeProvider provider = grid.getAccessibilityNodeProvider();
    assertNotNull(provider);
    assertEquals(120, provider.createAccessibilityNodeInfo(View.NO_ID).getChildCount());
    AccessibilityNodeInfo first = provider.createAccessibilityNodeInfo(0);
    assertEquals(Button.class.getName(), first.getClassName());
    assertTrue(first.getContentDescription().toString().contains("255"));
    Rect bounds = new Rect();
    first.getBoundsInParent(bounds);
    assertEquals(new Rect(12, 10, 32, 30), bounds);
    provider.createAccessibilityNodeInfo(119).getBoundsInParent(bounds);
    assertEquals(new Rect(232, 190, 252, 210), bounds);
  }

  @Test
  @Config(sdk = {23, 35})
  public void gridActionsUpdateSelectionAndRejectDisabledClicks()
  {
    ColorGridView grid = new ColorGridView(mContext);
    attach(grid);
    grid.layout(0, 0, 240, 200);
    List<Integer> changes = new ArrayList<>();
    grid.setOnColorChangedListener(changes::add);
    AccessibilityNodeProvider provider = grid.getAccessibilityNodeProvider();
    assertTrue(provider.performAction(0, AccessibilityNodeInfo.ACTION_CLICK, null));
    assertEquals(List.of(Color.WHITE), changes);
    assertTrue(provider.createAccessibilityNodeInfo(0).isSelected());
    grid.setColor(Color.BLACK);
    assertFalse(provider.createAccessibilityNodeInfo(0).isSelected());
    assertTrue(provider.createAccessibilityNodeInfo(11).isSelected());
    assertEquals(1, changes.size());
    grid.setEnabled(false);
    assertFalse(provider.performAction(1, AccessibilityNodeInfo.ACTION_CLICK, null));
    assertFalse(provider.createAccessibilityNodeInfo(1).isEnabled());
    assertEquals(1, changes.size());
  }

  @Test
  public void spectrumAccessibleClickOpensRgbButTouchDoesNot()
  {
    SpectrumColorView spectrum = new SpectrumColorView(mContext);
    attach(spectrum);
    spectrum.layout(0, 0, 240, 200);
    List<Integer> edits = new ArrayList<>();
    spectrum.setOnAccessibilityEditColor(() -> {
      edits.add(1);
      return kotlin.Unit.INSTANCE;
    });
    assertTrue(spectrum.performAccessibilityAction(AccessibilityNodeInfo.ACTION_CLICK, null));
    assertEquals(1, edits.size());
    MotionEvent down = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 50, 60, 0);
    MotionEvent up = MotionEvent.obtain(0, 10, MotionEvent.ACTION_UP, 50, 60, 0);
    spectrum.onTouchEvent(down);
    spectrum.onTouchEvent(up);
    down.recycle();
    up.recycle();
    assertEquals(1, edits.size());
    spectrum.setEnabled(false);
    assertFalse(spectrum.performAccessibilityAction(AccessibilityNodeInfo.ACTION_CLICK, null));
  }

  @Test
  public void keyboardCanSelectGridCells()
  {
    ColorGridView grid = new ColorGridView(mContext);
    attach(grid);
    grid.layout(0, 0, 240, 200);
    List<Integer> changes = new ArrayList<>();
    grid.setOnColorChangedListener(changes::add);
    grid.requestFocus();
    assertTrue(grid.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT)));
    assertTrue(grid.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER)));
    assertEquals(1, changes.size());
  }

  @Test
  public void spectrumKeyboardActivationRunsOnceOnKeyUp()
  {
    SpectrumColorView spectrum = new SpectrumColorView(mContext);
    attach(spectrum);
    List<Integer> edits = new ArrayList<>();
    spectrum.setOnAccessibilityEditColor(() -> {
      edits.add(1);
      return kotlin.Unit.INSTANCE;
    });
    assertTrue(spectrum.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER)));
    assertTrue(edits.isEmpty());
    assertTrue(spectrum.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER)));
    assertEquals(1, edits.size());
  }

  @Test
  public void spectrumStateReflectsProgrammaticColor()
  {
    SpectrumColorView spectrum = new SpectrumColorView(mContext);
    attach(spectrum);
    spectrum.setColor(Color.rgb(12, 34, 56));
    assertEquals("Red 12, green 34, blue 56", spectrum.createAccessibilityNodeInfo().getStateDescription());
  }
}
