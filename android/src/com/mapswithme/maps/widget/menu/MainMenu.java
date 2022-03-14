package com.mapswithme.maps.widget.menu;

import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.ClickMenuDelegate;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.UiUtils;

public class MainMenu extends BaseMenu
{
  public enum State
  {
    MENU
        {
          @Override
          boolean showToggle()
          {
            return true;
          }
        },
    NAVIGATION,
    ROUTE_PREPARE
        {
          @Override
          boolean showToggle()
          {
            return false;
          }
        };

    boolean showToggle()
    {
      return true;
    }
  }

  private final View mButtonsFrame;
  private final View mRoutePlanFrame;
  private final View mNewsMarker;

  private final MenuToggle mToggle;

  public enum Item implements BaseMenu.Item
  {
    MENU(R.id.toggle)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.MenuClickDelegate(activity, item);
          }
        },
    SEARCH(R.id.search)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.SearchClickDelegate(activity, item);
          }
        },
    HELP(R.id.help)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.HelpDelegate(activity, item);
          }
        },
    BOOKMARKS(R.id.bookmarks)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.BookmarksDelegate(activity, item);
          }
        },
    SETTINGS(R.id.settings)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            throw new UnsupportedOperationException("Main menu option doesn't support it!");
          }
        };

    private final int mViewId;

    Item(int viewId)
    {
      mViewId = viewId;
    }

    @Override
    public int getViewId()
    {
      return mViewId;
    }

    public void onClicked(@NonNull MwmActivity activity, @NonNull Item item)
    {
      ClickMenuDelegate delegate = createClickDelegate(activity, item);
      delegate.onMenuItemClick();
    }

    @NonNull
    public abstract ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                          @NonNull Item item);
  }

  private void mapItem(MainMenu.Item item)
  {
    mapItem(item, mButtonsFrame);
  }

  @Override
  void afterLayoutMeasured(Runnable procAfterCorrection)
  {
    UiUtils.showIf(!RoutingController.get().isNavigating(), mFrame);
    super.afterLayoutMeasured(procAfterCorrection);
  }

  @Override
  public void onResume(@Nullable Runnable procAfterMeasurement)
  {
    updateMarker();
  }

  @Override
  public boolean isOpen()
  {
    return false;
  }

  @Override
  public boolean isAnimating()
  {
    return false;
  }

  @Override
  public boolean open(boolean animate)
  {
    return false;
  }

  @Override
  public boolean close(boolean animate, @Nullable Runnable onCloseListener)
  {
    if (onCloseListener != null)
      onCloseListener.run();
    return false;
  }

  @Override
  public void toggle(boolean animate)
  {
    // Do nothing.
  }

  @Override
  public void updateMarker()
  {
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final boolean show = (count > 0 && !isOpen());

    UiUtils.showIf(show, mNewsMarker);

    // if (show)
    //   return;

    // for (Mode mode : Mode.values())
    // {
    //   show = SharedPropertiesUtils.shouldShowNewMarkerForLayerMode(mFrame.getContext(), mode);
    //   if (show)
    //     break;
    // }

    // UiUtils.showIf(show, mNewsMarker);
  }

  @Override
  protected void setToggleState(boolean open, boolean animate)
  {
    // Do nothing.
  }

  private void init()
  {
    mapItem(Item.SEARCH);
    mapItem(Item.HELP);
    mapItem(Item.BOOKMARKS);
    mapItem(Item.SETTINGS);

    setState(State.MENU, false);
  }

  public MainMenu(View frame, ItemClickListener<Item> itemClickListener)
  {
    super(frame, itemClickListener);

    mButtonsFrame = mLineFrame.findViewById(R.id.buttons_frame);
    mRoutePlanFrame = mLineFrame.findViewById(R.id.routing_plan_frame);
    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);

    mToggle = new MenuToggle(mLineFrame, getHeightResId());
    mapItem(Item.MENU, mLineFrame);

    init();
  }

  @Override
  protected int getHeightResId()
  {
    return R.dimen.menu_line_height;
  }

  public void setState(State state, boolean isFullScreen)
  {
    if (state != State.NAVIGATION)
    {
      mToggle.show(state.showToggle());
      mToggle.setOpen(false, false);

      boolean isRouting = state == State.ROUTE_PREPARE;
      if (mRoutePlanFrame == null)
      {
        UiUtils.show(mButtonsFrame);
      }
      else
      {
        UiUtils.showIf(state == State.MENU, mButtonsFrame);
        UiUtils.showIf(isRouting, mRoutePlanFrame);
        if (isRouting)
          mToggle.hide();
      }
    }

    show(state != State.NAVIGATION && !isFullScreen);
    UiUtils.showIf(state == State.MENU, mButtonsFrame);
    UiUtils.showIf(state == State.ROUTE_PREPARE, mRoutePlanFrame);
  }

  public void setEnabled(Item item, boolean enable)
  {
    View button = mButtonsFrame.findViewById(item.mViewId);
    if (button == null)
      return;

    button.setAlpha(enable ? 1.0f : 0.4f);
    button.setEnabled(enable);
  }

  private void setVisible(@NonNull Item item, boolean show)
  {
    final View itemInButtonsFrame = mButtonsFrame.findViewById(item.mViewId);
    if (itemInButtonsFrame != null)
      UiUtils.showIf(show, itemInButtonsFrame);
  }
}
