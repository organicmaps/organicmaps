package app.organicmaps.widget.menu;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.Button;
import android.widget.TextView;
import androidx.annotation.Nullable;
import androidx.core.os.ConfigurationCompat;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.widgets.speedlimit.Mode;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import java.text.NumberFormat;
import java.util.function.IntSupplier;
import java.util.function.Supplier;

public final class NavigationAccessibility
{
  private NavigationAccessibility() {}

  public static void attachPanel(View header, View background, IntSupplier state, Runnable expand, Runnable collapse)
  {
    header.setContentDescription(header.getContext().getString(R.string.navigation_controls));
    ViewCompat.setAccessibilityDelegate(header, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setClassName(Button.class.getName());
        int current = state.getAsInt();
        if (current != BottomSheetBehavior.STATE_EXPANDED && current != BottomSheetBehavior.STATE_COLLAPSED)
        {
          info.removeAction(AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_CLICK);
          return;
        }
        boolean expanded = current == BottomSheetBehavior.STATE_EXPANDED;
        info.addAction(expanded ? AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_COLLAPSE
                                : AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_EXPAND);
        info.removeAction(AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_CLICK);
        info.addAction(new AccessibilityNodeInfoCompat.AccessibilityActionCompat(
            AccessibilityNodeInfoCompat.ACTION_CLICK,
            host.getContext().getString(expanded ? R.string.navigation_controls_collapse
                                                 : R.string.navigation_controls_expand)));
      }

      @Override
      public boolean performAccessibilityAction(View host, int action, @Nullable Bundle arguments)
      {
        boolean expanded = state.getAsInt() == BottomSheetBehavior.STATE_EXPANDED;
        boolean collapsed = state.getAsInt() == BottomSheetBehavior.STATE_COLLAPSED;
        if (action == AccessibilityNodeInfoCompat.ACTION_EXPAND || action == AccessibilityNodeInfoCompat.ACTION_COLLAPSE
            || action == AccessibilityNodeInfoCompat.ACTION_CLICK)
        {
          if (!host.isEnabled() || (!expanded && !collapsed))
            return false;
          if (action == AccessibilityNodeInfoCompat.ACTION_CLICK)
            return host.performClick();
          if (collapsed && action != AccessibilityNodeInfoCompat.ACTION_COLLAPSE)
            expand.run();
          else if (expanded && action != AccessibilityNodeInfoCompat.ACTION_EXPAND)
            collapse.run();
          else
            return false;
          return true;
        }
        return super.performAccessibilityAction(host, action, arguments);
      }
    });
    background.setContentDescription(background.getContext().getString(R.string.navigation_controls_collapse));
    ViewCompat.setAccessibilityDelegate(background, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setClassName(Button.class.getName());
      }
    });
    updatePanelState(header, state.getAsInt());
  }

  public static void updatePanelState(View header, int state)
  {
    if (state != BottomSheetBehavior.STATE_EXPANDED && state != BottomSheetBehavior.STATE_COLLAPSED)
      return;
    Object previous = header.getTag(R.id.navigation_accessibility_state);
    if (previous instanceof Integer && (Integer) previous == state)
      return;
    header.setTag(R.id.navigation_accessibility_state, state);
    // Standard expansion actions already convey the state to TalkBack.
    header.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
  }

  /** Query current values instead of publishing a new description on every GPS update. */
  public static void attachReading(View view, Supplier<CharSequence> description)
  {
    ViewCompat.setImportantForAccessibility(view, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
    ViewCompat.setScreenReaderFocusable(view, true);
    if (view instanceof ViewGroup group)
      for (int i = 0; i < group.getChildCount(); ++i)
        ViewCompat.setImportantForAccessibility(group.getChildAt(i),
                                                ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
    ViewCompat.setAccessibilityDelegate(view, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setContentDescription(description.get());
      }
    });
  }

  public static void attachNumberGroup(View group, int label, TextView... values)
  {
    attachReading(group, () -> {
      StringBuilder text = new StringBuilder();
      for (TextView value : values)
      {
        if (value.getVisibility() != View.VISIBLE || value.getText().length() == 0)
          continue;
        if (text.length() > 0)
          text.append(' ');
        text.append(value.getText());
      }
      return group.getContext().getString(label, text);
    });
  }

  public static CharSequence speedDescription(Context context, String speed, String units, @Nullable Integer limit,
                                              Mode mode)
  {
    String description = context.getString(R.string.navigation_current_speed, speed, units);
    if (limit != null)
      description += ". " + context.getString(R.string.navigation_speed_limit, formatNumber(context, limit), units);
    if (mode != Mode.Normal)
      description +=
          ". "
          + context.getString(mode == Mode.Alert ? R.string.navigation_speed_alert : R.string.navigation_speed_warning);
    return description;
  }

  public static CharSequence speedDescription(Context context, int speed, String units, @Nullable Integer limit,
                                              Mode mode)
  {
    return speedDescription(context, formatNumber(context, speed), units, limit, mode);
  }

  private static String formatNumber(Context context, int number)
  {
    return NumberFormat
        .getIntegerInstance(ConfigurationCompat.getLocales(context.getResources().getConfiguration()).get(0))
        .format(number);
  }
}
