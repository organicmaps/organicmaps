package com.mapswithme.maps.widget.menu;

import android.animation.Animator;
import android.app.Activity;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainMenu
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

  public static final int ANIMATION_DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_menu);
  private static final String TAG_COLLAPSE = MwmApplication.get().getString(R.string.tag_menu_collapse);

  private final int mButtonsWidth = UiUtils.dimen(R.dimen.menu_line_button_width);
  private final int mPanelWidth = UiUtils.dimen(R.dimen.panel_width);

  private final Container mContainer;
  private final ViewGroup mFrame;
  private final View mButtonsFrame;
  private final View mRoutePlanFrame;
  private final View mContentFrame;
  private final View mAnimationSpacer;
  private final View mAnimationSymmetricalGap;
  private final View mNewsMarker;

  private final TextView mNewsCounter;

  private boolean mCollapsed;
  private final List<View> mCollapseViews = new ArrayList<>();

  private final MyPositionButton mMyPositionButton;
  private final MenuToggle mToggle;
  private Button mRouteStartButton;

  private int mContentHeight;

  // Maps Item into button view placed on mContentFrame
  private final Map<Item, View> mItemViews = new HashMap<>();

  private boolean mLayoutCorrected;
  private boolean mAnimating;

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

  public enum Item
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
  }

  public interface Container
  {
    Activity getActivity();
    void onItemClick(Item item);
  }

  private class AnimationListener extends UiUtils.SimpleAnimatorListener
  {
    @Override
    public void onAnimationStart(android.animation.Animator animation)
    {
      mAnimating = true;
    }

    @Override
    public void onAnimationEnd(android.animation.Animator animation)
    {
      mAnimating = false;
    }
  }

  private View mapItem(final Item item, View frame)
  {
    View res = frame.findViewById(item.mViewId);
    if (res != null)
    {
      if (TAG_COLLAPSE.equals(res.getTag()))
        mCollapseViews.add(res);

      res.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          mContainer.onItemClick(item);
        }
      });
    }
    return res;
  }

  private void mapItem(Item item)
  {
    mapItem(item, mButtonsFrame);
    View view = mapItem(item, mContentFrame);
    mItemViews.put(item, view);

    if (view != null)
      Graphics.tint((TextView)view);
  }

  private void adjustCollapsedItems()
  {
    for (View v : mCollapseViews)
    {
      UiUtils.showIf(!mCollapsed, v);
      v.setAlpha(mCollapsed ? 0.0f : 1.0f);
    }

    if (mAnimationSymmetricalGap == null)
      return;

    LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) mAnimationSymmetricalGap.getLayoutParams();
    lp.width = (mCollapsed ? 0 : mButtonsWidth);
    mAnimationSymmetricalGap.setLayoutParams(lp);
  }

  private void adjustTransparency()
  {
    mFrame.setBackgroundColor(ThemeUtils.getColor(mFrame.getContext(), isOpen() ? R.attr.menuBackgroundOpen
                                                                                : R.attr.menuBackgroundClosed));
  }

  private void correctLayout(final Runnable procAfterCorrection)
  {
    if (mLayoutCorrected)
      return;

    UiUtils.measureView(mContentFrame, new UiUtils.OnViewMeasuredListener()
    {
      @Override
      public void onViewMeasured(int width, int height)
      {
        mContentHeight = height;
        mLayoutCorrected = true;

        UiUtils.hide(mContentFrame);
        UiUtils.showIf(!RoutingController.get().isNavigating(), mFrame);

        procAfterCorrection.run();
      }
    });
  }

  private void updateMarker()
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

  public void onResume(Runnable procAfterCorrection)
  {
    correctLayout(procAfterCorrection);
    updateMarker();
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
    adjustTransparency();
    setState(State.MENU, false);
  }

  public MainMenu(ViewGroup frame, Container container)
  {
    mContainer = container;
    mFrame = frame;

    View lineFrame = mFrame.findViewById(R.id.line_frame);
    mButtonsFrame = lineFrame.findViewById(R.id.buttons_frame);
    mRoutePlanFrame = lineFrame.findViewById(R.id.routing_plan_frame);
    mContentFrame = mFrame.findViewById(R.id.content_frame);

    mAnimationSpacer = mFrame.findViewById(R.id.animation_spacer);
    mAnimationSymmetricalGap = mButtonsFrame.findViewById(R.id.symmetrical_gap);

    mMyPositionButton = new MyPositionButton(lineFrame.findViewById(R.id.my_position));
    mToggle = new MenuToggle(lineFrame);
    mapItem(Item.TOGGLE, lineFrame);

    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);
    mNewsCounter = (TextView) mContentFrame.findViewById(R.id.counter);

    if (mRoutePlanFrame != null)
      mRouteStartButton = (Button) mRoutePlanFrame.findViewById(R.id.start);

    init();
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
      UiUtils.showIf(state != State.NAVIGATION, mFrame);
      mContentFrame.measure(ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);
      mContentHeight = mContentFrame.getMeasuredHeight();
    }
  }

  public boolean isOpen()
  {
    return UiUtils.isVisible(mContentFrame);
  }

  public boolean open(boolean animate)
  {
    if ((animate && mAnimating) || isOpen())
      return false;

    UiUtils.show(mContentFrame);
    adjustCollapsedItems();
    adjustTransparency();
    updateMarker();

    mToggle.setOpen(true, animate);
    if (!animate)
      return true;

    mFrame.setTranslationY(mContentHeight);
    mFrame.animate()
          .setDuration(ANIMATION_DURATION)
          .translationY(0.0f)
          .setListener(new AnimationListener())
          .start();

    return true;
  }

  public boolean close(boolean animate)
  {
    return close(animate, null);
  }

  public boolean close(boolean animate, @Nullable final Runnable onCloseListener)
  {
    if (mAnimating || !isOpen())
    {
      if (onCloseListener != null)
        onCloseListener.run();

      return false;
    }

    adjustCollapsedItems();

    if (!animate)
    {
      UiUtils.hide(mContentFrame);
      adjustTransparency();
      updateMarker();

      mToggle.setOpen(false, false);

      if (onCloseListener != null)
        onCloseListener.run();

      return true;
    }

    mToggle.setOpen(false, true);

    mFrame.animate()
          .setDuration(ANIMATION_DURATION)
          .translationY(mContentHeight)
          .setListener(new AnimationListener()
          {
            @Override
            public void onAnimationEnd(Animator animation)
            {
              super.onAnimationEnd(animation);

              mFrame.setTranslationY(0.0f);
              UiUtils.hide(mContentFrame);
              adjustTransparency();
              updateMarker();

              if (onCloseListener != null)
                onCloseListener.run();
            }
          }).start();

    return true;
  }

  public void toggle(boolean animate)
  {
    if (mAnimating)
      return;

    boolean show = !isOpen();

    if (show)
      open(animate);
    else
      close(animate);
  }

  public void show(boolean show)
  {
    close(false);
    UiUtils.showIf(show, mFrame);
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

  public View getFrame()
  {
    return mFrame;
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

  public boolean isAnimating()
  {
    return mAnimating;
  }
}
