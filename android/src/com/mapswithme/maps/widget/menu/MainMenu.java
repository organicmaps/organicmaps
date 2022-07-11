package com.mapswithme.maps.widget.menu;

import android.view.View;

import com.mapswithme.util.UiUtils;

public class MainMenu
{
  private final View mFrame;
  private final OnMenuSizeChangeListener mOnMenuSizeChangeListener;
  private int mMenuHeight;

  public MainMenu(View frame, OnMenuSizeChangeListener onMenuSizeChangeListener)
  {
    mFrame = frame;
    mOnMenuSizeChangeListener = onMenuSizeChangeListener;
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
    notifyHeight();
  }

  private void notifyHeight()
  {
    mOnMenuSizeChangeListener.OnMenuSizeChange(UiUtils.isVisible(mFrame) ? mMenuHeight : 0);
  }

  public View getFrame()
  {
    return mFrame;
  }

  public enum State
  {
    MENU,
    NAVIGATION,
    ROUTE_PREPARE
  }

  public enum Item
  {
    MENU,
    SEARCH,
    HELP,
    BOOKMARKS
  }

  public interface OnMenuSizeChangeListener
  {
    void OnMenuSizeChange(int newHeight);
  }

  private class FrameLayoutChangeListener implements View.OnLayoutChangeListener
  {
    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                               int oldTop, int oldRight, int oldBottom)
    {
      mMenuHeight = bottom - top;
      notifyHeight();
    }
  }
}
