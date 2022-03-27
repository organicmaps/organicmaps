package com.mapswithme.maps.widget.menu;

import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.util.UiUtils;

public class MainMenu
{
  private final View mFrame;
  private final View mButtonsFrame;
  private final View mRoutePlanFrame;
  private final View mNewsMarker;
  private final ItemClickListener mItemClickListener;

  public MainMenu(View frame, ItemClickListener itemClickListener)
  {
    mFrame = frame;
    mItemClickListener = itemClickListener;

    mButtonsFrame = mFrame.findViewById(R.id.buttons_frame);
    mRoutePlanFrame = mFrame.findViewById(R.id.routing_plan_frame);
    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);
    init();
  }

  void mapItem(Item item, int viewId)
  {
    View res = mButtonsFrame.findViewById(viewId);
    if (res != null)
      res.setOnClickListener(v -> mItemClickListener.onItemClick(item));
  }

  public void onResume()
  {
    updateMarker();
  }

  public void updateMarker()
  {
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final boolean show = count > 0;

    UiUtils.showIf(show, mNewsMarker);
  }

  private void init()
  {
    mapItem(Item.HELP, R.id.help);
    mapItem(Item.SEARCH, R.id.search);
    mapItem(Item.BOOKMARKS, R.id.bookmarks);
    mapItem(Item.MENU, R.id.toggle);

    setState(State.MENU, false);
  }

  public void setState(State state, boolean isFullScreen)
  {
    if (state != State.NAVIGATION)
    {
      boolean isRouting = state == State.ROUTE_PREPARE;
      if (mRoutePlanFrame == null)
        UiUtils.show(mButtonsFrame);
      else
      {
        UiUtils.showIf(state == State.MENU, mButtonsFrame);
        UiUtils.showIf(isRouting, mRoutePlanFrame);
      }
    }

    show(state != State.NAVIGATION && !isFullScreen);
    UiUtils.showIf(state == State.MENU, mButtonsFrame);
    UiUtils.showIf(state == State.ROUTE_PREPARE, mRoutePlanFrame);
  }

  public void show(boolean show)
  {
    if (show && mFrame.isShown())
      return;

    UiUtils.showIf(show, mFrame);
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

  public interface ItemClickListener
  {
    void onItemClick(Item item);
  }
}
