package app.organicmaps.widget;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import java.util.function.BooleanSupplier;

/** Adds expansion semantics while keeping the view's existing click handler and validation. */
public final class ExpandableSectionAccessibility extends AccessibilityDelegateCompat
{
  private final View mView;
  private final BooleanSupplier mCanExpand;
  private final BooleanSupplier mIsExpanded;

  public ExpandableSectionAccessibility(View view, BooleanSupplier canExpand, BooleanSupplier isExpanded)
  {
    mView = view;
    mCanExpand = canExpand;
    mIsExpanded = isExpanded;
    ViewCompat.setAccessibilityDelegate(view, this);
  }

  public void update()
  {
    boolean canExpand = mCanExpand.getAsBoolean();
    mView.setClickable(canExpand);
    mView.setFocusable(canExpand);
    ViewCompat.setStateDescription(
        mView, canExpand ? mView.getContext().getString(mIsExpanded.getAsBoolean() ? R.string.editor_section_expanded
                                                                                   : R.string.editor_section_collapsed)
                         : null);
  }

  @Override
  public void onInitializeAccessibilityNodeInfo(@NonNull View host, @NonNull AccessibilityNodeInfoCompat info)
  {
    super.onInitializeAccessibilityNodeInfo(host, info);
    if (!mCanExpand.getAsBoolean())
      return;
    info.setClassName(Button.class.getName());
    if (host.isEnabled())
      info.addAction(mIsExpanded.getAsBoolean() ? AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_COLLAPSE
                                                : AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_EXPAND);
  }

  @Override
  public boolean performAccessibilityAction(@NonNull View host, int action, @Nullable Bundle arguments)
  {
    if (action != AccessibilityNodeInfoCompat.ACTION_EXPAND && action != AccessibilityNodeInfoCompat.ACTION_COLLAPSE
        && action != AccessibilityNodeInfoCompat.ACTION_CLICK)
      return super.performAccessibilityAction(host, action, arguments);
    if (!host.isEnabled() || !mCanExpand.getAsBoolean())
      return false;
    if (action == AccessibilityNodeInfoCompat.ACTION_CLICK)
      return host.performClick();
    boolean expand = action == AccessibilityNodeInfoCompat.ACTION_EXPAND;
    if (expand == mIsExpanded.getAsBoolean())
      return false;
    host.performClick();
    return expand == mIsExpanded.getAsBoolean();
  }
}
