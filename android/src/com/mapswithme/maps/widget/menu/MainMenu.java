package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.ClickMenuDelegate;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.StatisticValueConverter;

import java.util.Locale;

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

  public enum Item implements BaseMenu.Item, StatisticValueConverter<String>
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
    ADD_PLACE(R.id.add_place)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            throw new UnsupportedOperationException("Main menu option doesn't support it!");
          }
        },
    DOWNLOAD_GUIDES(R.id.download_guides)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            throw new UnsupportedOperationException("Main menu option doesn't support it!");
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
    POINT_TO_POINT(R.id.p2p)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.PointToPointDelegate(activity, item);
          }
        },
    DISCOVERY(R.id.discovery)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            return new MwmActivity.DiscoveryDelegate(activity, item);
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
    SHARE_MY_LOCATION(R.id.share)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            throw new UnsupportedOperationException("Main menu option doesn't support it!");
          }
        },
    DOWNLOAD_MAPS(R.id.download_maps)
        {
          @NonNull
          @Override
          public ClickMenuDelegate createClickDelegate(@NonNull MwmActivity activity,
                                                       @NonNull Item item)
          {
            throw new UnsupportedOperationException("Main menu option doesn't support it!");
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

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return name().toLowerCase(Locale.ENGLISH);
    }
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
    UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    int count = (info == null ? 0 : info.filesCount);

    boolean show = (count > 0 && !isOpen());

    UiUtils.showIf(show, mNewsMarker);

    if (show)
      return;

    for (Mode mode : Mode.values())
    {
      show = SharedPropertiesUtils.shouldShowNewMarkerForLayerMode(mFrame.getContext(), mode);
      if (show)
        break;
    }

    UiUtils.showIf(show, mNewsMarker);
  }

  @Override
  protected void setToggleState(boolean open, boolean animate)
  {
    // Do nothing.
  }

  private void init()
  {
    mapItem(Item.ADD_PLACE);
    mapItem(Item.DOWNLOAD_GUIDES);
    mapItem(Item.SEARCH);
    mapItem(Item.POINT_TO_POINT);
    mapItem(Item.DISCOVERY);
    mapItem(Item.BOOKMARKS);
    mapItem(Item.SHARE_MY_LOCATION);
    mapItem(Item.DOWNLOAD_MAPS);
    mapItem(Item.SETTINGS);

    setState(State.MENU, false, false);
  }

  public MainMenu(View frame, ItemClickListener<Item> itemClickListener)
  {
    super(frame, itemClickListener);

    mButtonsFrame = mLineFrame.findViewById(R.id.buttons_frame);
    mRoutePlanFrame = mLineFrame.findViewById(R.id.routing_plan_frame);

    mToggle = new MenuToggle(mLineFrame, getHeightResId());
    mapItem(Item.MENU, mLineFrame);

    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);

    init();
  }

  @Override
  protected int getHeightResId()
  {
    return R.dimen.menu_line_height;
  }

  public void setState(State state, boolean animateToggle, boolean isFullScreen)
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

      setVisible(Item.ADD_PLACE, !isRouting);
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
