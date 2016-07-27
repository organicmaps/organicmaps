package com.mapswithme.maps.widget.menu;

import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainMenu extends BaseMenu
{
  public enum State
  {
    MENU
    {
      @Override
      boolean showToggle()
      {
        return false;
      }
    },
    NAVIGATION,
    ROUTE_PREPARE;

    boolean showToggle()
    {
      return true;
    }
  }

  private static final String TAG_COLLAPSE = MwmApplication.get().getString(R.string.tag_menu_collapse);

  private final int mButtonsWidth = UiUtils.dimen(R.dimen.menu_line_button_width);
  private final int mPanelWidth = UiUtils.dimen(R.dimen.panel_width);

  private final View mButtonsFrame;
  private final View mRoutePlanFrame;
  private final View mAnimationSpacer;
  private final View mAnimationSymmetricalGap;
  private final View mNewsMarker;

  private final TextView mNewsCounter;

  private boolean mCollapsed;
  private final List<View> mCollapseViews = new ArrayList<>();

  private final MyPositionButton mMyPositionButton;
  private final MenuToggle mToggle;
  private Button mRouteStartButton;

  // Maps Item into button view placed on mContentFrame
  private final Map<Item, View> mItemViews = new HashMap<>();

  private final MwmActivity.LeftAnimationTrackListener mAnimationTrackListener = new MwmActivity.LeftAnimationTrackListener()
  {
    private float mSymmetricalGapScale;

    @Override
    public void onTrackStarted(boolean collapsed)
    {
      for (View v : mCollapseViews)
      {
        if (collapsed)
          UiUtils.show(v);

        v.setAlpha(collapsed ? 0.0f : 1.0f);
        v.animate()
         .alpha(collapsed ? 1.0f : 0.0f)
         .start();
      }

      mToggle.setCollapsed(!collapsed, true);

      mSymmetricalGapScale = (float) mButtonsWidth / mPanelWidth;
    }

    @Override
    public void onTrackFinished(boolean collapsed)
    {
      mCollapsed = collapsed;
      updateMarker();

      if (collapsed)
        for (View v : mCollapseViews)
          UiUtils.hide(v);
    }

    @Override
    public void onTrackLeftAnimation(float offset)
    {
      ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mAnimationSpacer.getLayoutParams();
      lp.rightMargin = (int) offset;
      mAnimationSpacer.setLayoutParams(lp);

      if (mAnimationSymmetricalGap == null)
        return;

      lp = (ViewGroup.MarginLayoutParams) mAnimationSymmetricalGap.getLayoutParams();
      lp.width = mButtonsWidth - (int) (mSymmetricalGapScale * offset);
      mAnimationSymmetricalGap.setLayoutParams(lp);
    }
  };

  public enum Item implements BaseMenu.Item
  {
    TOGGLE(R.id.toggle),
    ADD_PLACE(R.id.add_place),
    SEARCH(R.id.search),
    P2P(R.id.p2p),
    BOOKMARKS(R.id.bookmarks),
    SHARE(R.id.share),
    DOWNLOADER(R.id.download_maps),
    SETTINGS(R.id.settings),
    SHOWCASE(R.id.showcase);

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
  }

  @Override
  View mapItem(BaseMenu.Item item, View frame)
  {
    View res = super.mapItem(item, frame);
    if (res != null && TAG_COLLAPSE.equals(res.getTag()))
      mCollapseViews.add(res);

    return res;
  }

  private void mapItem(MainMenu.Item item)
  {
    mapItem(item, mButtonsFrame);
    View view = mapItem(item, mContentFrame);
    mItemViews.put(item, view);

    if (view != null)
      Graphics.tint((TextView)view);
  }

  @Override
  protected void adjustCollapsedItems()
  {
    for (View v : mCollapseViews)
    {
      UiUtils.showIf(!mCollapsed, v);
      v.setAlpha(mCollapsed ? 0.0f : 1.0f);
    }

    if (mAnimationSymmetricalGap == null)
      return;

    ViewGroup.LayoutParams lp = mAnimationSymmetricalGap.getLayoutParams();
    lp.width = (mCollapsed ? 0 : mButtonsWidth);
    mAnimationSymmetricalGap.setLayoutParams(lp);
  }

  @Override
  void afterLayoutCorrected(Runnable procAfterCorrection)
  {
    UiUtils.showIf(!RoutingController.get().isNavigating(), mFrame);
    super.afterLayoutCorrected(procAfterCorrection);
  }

  @Override
  protected void updateMarker()
  {
    UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    int count = (info == null ? 0 : info.filesCount);

    boolean show = (MapManager.nativeIsLegacyMode() || count > 0) &&
                   (!mCollapsed || mCollapseViews.isEmpty()) &&
                   !isOpen();

    UiUtils.showIf(show, mNewsMarker);
    UiUtils.showIf(count > 0, mNewsCounter);

    if (count > 0)
      mNewsCounter.setText(String.valueOf(count));
  }

  @Override
  protected void setToggleState(boolean open, boolean animate)
  {
    mToggle.setOpen(open, animate);
  }

  private void init()
  {
    mapItem(Item.ADD_PLACE);
    mapItem(Item.SEARCH);
    mapItem(Item.P2P);
    mapItem(Item.BOOKMARKS);
    mapItem(Item.SHARE);
    mapItem(Item.DOWNLOADER);
    mapItem(Item.SETTINGS);
    mapItem(Item.SHOWCASE);

    adjustCollapsedItems();
    setState(State.MENU, false);
  }

  public MainMenu(View frame, ItemClickListener<Item> itemClickListener)
  {
    super(frame, itemClickListener);

    mButtonsFrame = mLineFrame.findViewById(R.id.buttons_frame);
    mRoutePlanFrame = mLineFrame.findViewById(R.id.routing_plan_frame);

    mAnimationSpacer = mFrame.findViewById(R.id.animation_spacer);
    mAnimationSymmetricalGap = mButtonsFrame.findViewById(R.id.symmetrical_gap);

    mMyPositionButton = new MyPositionButton(mLineFrame.findViewById(R.id.my_position));
    mToggle = new MenuToggle(mLineFrame, getHeightResId());
    mapItem(Item.TOGGLE, mLineFrame);

    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);
    mNewsCounter = (TextView) mContentFrame.findViewById(R.id.counter);

    if (mRoutePlanFrame != null)
      mRouteStartButton = (Button) mRoutePlanFrame.findViewById(R.id.start);

    init();
  }

  @Override
  protected int getHeightResId()
  {
    return R.dimen.menu_line_height;
  }

  public void setState(State state, boolean animateToggle)
  {
    if (state != State.NAVIGATION)
    {
      mToggle.show(state.showToggle());
      mToggle.setCollapsed(mCollapsed, animateToggle);

      boolean expandContent;
      boolean isRouting = state == State.ROUTE_PREPARE;
      if (mRoutePlanFrame == null)
      {
        UiUtils.show(mButtonsFrame);
        expandContent = false;
      } else
      {
        UiUtils.showIf(state == State.MENU, mButtonsFrame);
        UiUtils.showIf(state == State.ROUTE_PREPARE, mRoutePlanFrame);
        expandContent = isRouting;
      }

      UiUtils.showIf(expandContent,
                     mItemViews.get(Item.SEARCH),
                     mItemViews.get(Item.BOOKMARKS));
      setVisible(Item.ADD_PLACE, !isRouting && !MapManager.nativeIsLegacyMode());
    }

    if (mLayoutCorrected)
    {
      show(state != State.NAVIGATION);
      mContentFrame.measure(ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);
      mContentHeight = mContentFrame.getMeasuredHeight();
    }
  }

  public void setEnabled(Item item, boolean enable)
  {
    View button = mButtonsFrame.findViewById(item.mViewId);
    if (button == null)
      return;

    button.setAlpha(enable ? 1.0f : 0.4f);
    button.setEnabled(enable);
  }

  public void setVisible(Item item, boolean show)
  {
    final View itemInButtonsFrame = mButtonsFrame.findViewById(item.mViewId);
    if (itemInButtonsFrame != null)
      UiUtils.showIf(show, itemInButtonsFrame);
    if (mItemViews.get(item) != null)
      UiUtils.showIf(show, mItemViews.get(item));
  }


  public MyPositionButton getMyPositionButton()
  {
    return mMyPositionButton;
  }

  public MwmActivity.LeftAnimationTrackListener getLeftAnimationTrackListener()
  {
    return mAnimationTrackListener;
  }

  public void setShowcaseText(String text)
  {
    ((TextView) mItemViews.get(Item.SHOWCASE)).setText(text);
  }

  public Button getRouteStartButton()
  {
    return mRouteStartButton;
  }
}
