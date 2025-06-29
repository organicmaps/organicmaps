package app.organicmaps.widget.menu;

import android.view.View;
import app.organicmaps.sdk.util.UiUtils;

public class MainMenu
{
  private final View mFrame;
  private final OnMenuSizeChangedListener mOnMenuSizeChangedListener;
  private int mMenuHeight;

  public MainMenu(View frame, OnMenuSizeChangedListener onMenuSizeChangedListener)
  {
    mFrame = frame;
    mOnMenuSizeChangedListener = onMenuSizeChangedListener;
    mFrame.addOnLayoutChangeListener(new MainMenu.FrameLayoutChangeListener());
    setState(State.MENU, false);
  }

  public void setState(State state, boolean isFullScreen)
  {
    show(state == State.ROUTE_PREPARE && !isFullScreen);
  }

  public void show(boolean show)
  {
    if (show && mFrame.isShown())
      return;

    UiUtils.showIf(show, mFrame);
    mOnMenuSizeChangedListener.OnMenuSizeChange(show);
  }

  public int getMenuHeight()
  {
    return UiUtils.isVisible(mFrame) ? mMenuHeight : 0;
  }

  public enum State
  {
    MENU,
    NAVIGATION,
    ROUTE_PREPARE
  }

  public interface OnMenuSizeChangedListener
  {
    void OnMenuSizeChange(boolean visible);
  }

  private class FrameLayoutChangeListener implements View.OnLayoutChangeListener
  {
    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                               int oldBottom)
    {
      mMenuHeight = bottom - top;
      mOnMenuSizeChangedListener.OnMenuSizeChange(UiUtils.isVisible(mFrame));
    }
  }
}
