package app.organicmaps.widget;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.customview.widget.ExploreByTouchHelper;
import app.organicmaps.sdk.MapView;

/** Forwards actual keyboard entry direction while retaining the SDK map surface. */
public class AccessibleMapView extends MapView
{
  @Nullable
  private ExploreByTouchHelper mAccessibilityHelper;

  public AccessibleMapView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  void setAccessibilityHelper(ExploreByTouchHelper helper)
  {
    mAccessibilityHelper = helper;
  }

  @Override
  protected void onFocusChanged(boolean gainFocus, int direction, @Nullable Rect previouslyFocusedRect)
  {
    super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
    if (mAccessibilityHelper != null)
      mAccessibilityHelper.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
  }
}
