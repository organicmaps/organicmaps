package app.organicmaps.util;

import android.app.Activity;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;

public class KeyboardVisibilityHelper
{

  public interface KeyboardVisibilityListener
  {
    void onKeyboardVisibilityChanged(boolean isVisible);
  }

  private final View rootView;
  private boolean isKeyboardVisible;
  private final ViewTreeObserver.OnGlobalLayoutListener layoutListener;

  public KeyboardVisibilityHelper(Activity activity, KeyboardVisibilityListener listener)
  {
    this.rootView = activity.getWindow().getDecorView().getRootView();
    this.layoutListener = () ->
    {
      Rect r = new Rect();
      rootView.getWindowVisibleDisplayFrame(r);
      int screenHeight = rootView.getHeight();

      // determine visible area size
      int keypadHeight = screenHeight - r.bottom;

      // if the height of the visible area exceeds a certain threshold, we assume the keyboard is displayed.
      boolean isVisible = keypadHeight > screenHeight * 0.15;

      if (isVisible != isKeyboardVisible)
      {
        isKeyboardVisible = isVisible;
        listener.onKeyboardVisibilityChanged(isVisible);
      }
    };

    rootView.getViewTreeObserver().addOnGlobalLayoutListener(layoutListener);
  }

  public void detach()
  {
    rootView.getViewTreeObserver().removeOnGlobalLayoutListener(layoutListener);
  }
}