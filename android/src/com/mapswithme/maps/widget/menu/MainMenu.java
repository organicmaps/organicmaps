package com.mapswithme.maps.widget.menu;

import android.animation.Animator;
import android.app.Activity;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.maps.widget.RotateByAlphaDrawable;
import com.mapswithme.maps.widget.TrackedTransitionDrawable;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainMenu
{
  private static final int ANIMATION_DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_menu);
  private static final String TAG_COLLAPSE = MwmApplication.get().getString(R.string.tag_menu_collapse);
  private static final String STATE_OPEN = "MenuOpen";
  private static final String STATE_LAST_INFO = "MenuLastInfo";

  private final int mButtonsHeight = UiUtils.dimen(R.dimen.menu_line_height);
  private final int mPanelWidth = UiUtils.dimen(R.dimen.panel_width);


  private final Container mContainer;
  private final ViewGroup mFrame;
  private final View mButtonsFrame;
  private final View mNavigationFrame;
  private final View mContentFrame;
  private final View mAnimationSpacer;
  private final View mAnimationSymmetricalGap;
  private final View mNewsMarker;

  private final TextView mCurrentPlace;
  private final TextView mNewsCounter;

  private boolean mLayoutCorrected;
  private boolean mCollapsed;
  private boolean mOpenDelayed;
  private final List<View> mCollapseViews = new ArrayList<>();

  private final MyPositionButton mMyPositionButton;
  private final Toggle mToggle;

  private int mContentHeight;

  // Maps Item into button view placed on mContentFrame
  private final Map<Item, View> mItemViews = new HashMap<>();

  private boolean mAnimating;


  private final MwmActivity.LeftAnimationTrackListener mAnimationTrackListener = new MwmActivity.LeftAnimationTrackListener()
  {
    private float mSymmetricalGapScale;


    @Override
    public void onTrackStarted(boolean collapsed)
    {
      for (View v: mCollapseViews)
      {
        v.setAlpha(collapsed ? 0.0f : 1.0f);
        v.animate()
         .alpha(collapsed ? 1.0f : 0.0f)
         .start();
      }

      mToggle.animateCollapse(!collapsed);

      mSymmetricalGapScale = (float) mButtonsHeight / mPanelWidth;
    }

    @Override
    public void onTrackFinished(boolean collapsed)
    {
      mCollapsed = collapsed;
      updateMarker();
    }

    @Override
    public void onTrackLeftAnimation(float offset)
    {
      LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) mAnimationSpacer.getLayoutParams();
      lp.rightMargin = (int) offset;
      mAnimationSpacer.setLayoutParams(lp);

      if (mAnimationSymmetricalGap == null)
        return;

      lp = (LinearLayout.LayoutParams) mAnimationSymmetricalGap.getLayoutParams();
      lp.width = mButtonsHeight - (int) (mSymmetricalGapScale * offset);
      mAnimationSymmetricalGap.setLayoutParams(lp);
    }
  };


  public enum Item
  {
    TOGGLE(R.id.toggle),
    SEARCH(R.id.search),
    ROUTE(R.id.route),
    BOOKMARKS(R.id.bookmarks),
    SHARE(R.id.share),
    DOWNLOADER(R.id.download_maps),
    SETTINGS(R.id.settings);

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


  private class AnimationListener implements Animator.AnimatorListener
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

    @Override
    public void onAnimationCancel(android.animation.Animator animation)
    {}

    @Override
    public void onAnimationRepeat(android.animation.Animator animation)
    {}
  }


  private class Toggle
  {
    final ImageView mButton;
    final boolean mAlwaysShow;

    final TransitionDrawable mOpenImage;
    final TransitionDrawable mCollapseImage;


    public Toggle(View frame)
    {
      mButton = (ImageView) frame.findViewById(R.id.toggle);
      mAlwaysShow = (mFrame.findViewById(R.id.disable_toggle) == null);
      mapItem(Item.TOGGLE, frame);

      @SuppressWarnings("SuspiciousNameCombination")
      Rect bounds = new Rect(0, 0, mButtonsHeight, mButtonsHeight);

      mOpenImage = new TrackedTransitionDrawable(new Drawable[] { new RotateByAlphaDrawable(R.drawable.ic_menu_open, false)
                                                                      .setInnerBounds(bounds),
                                                                  new RotateByAlphaDrawable(R.drawable.ic_menu_close, true)
                                                                      .setInnerBounds(bounds)
                                                                      .setBaseAngle(-90) });
      mCollapseImage = new TrackedTransitionDrawable(new Drawable[] { new RotateByAlphaDrawable(R.drawable.ic_menu_open, false)
                                                                          .setInnerBounds(bounds),
                                                                      new RotateByAlphaDrawable(R.drawable.ic_menu_close, true)
                                                                          .setInnerBounds(bounds) });
      mOpenImage.setCrossFadeEnabled(true);
      mCollapseImage.setCrossFadeEnabled(true);
    }

    public void updateNavigationMode(boolean navigation)
    {
      UiUtils.showIf(mAlwaysShow || navigation, mButton);
      animateCollapse(mCollapsed);
    }

    private void transitImage(TransitionDrawable image, boolean forward, boolean animate)
    {
      if (mButton.getVisibility() != View.VISIBLE)
        animate = false;

      mButton.setImageDrawable(image);

      if (forward)
        image.startTransition(animate ? ANIMATION_DURATION : 0);
      else
        image.reverseTransition(animate ? ANIMATION_DURATION : 0);

      if (!animate)
        image.getDrawable(forward ? 1 : 0).setAlpha(0xFF);
    }

    public void updateOpenImageNoAnimation(boolean open)
    {
      transitImage(mOpenImage, open, false);
    }

    public void animateOpen(boolean open)
    {
      transitImage(mOpenImage, open, true);
    }

    public void animateCollapse(boolean collapse)
    {
      transitImage(mCollapseImage, collapse, true);
    }
  }


  private View mapItem(final Item item, View frame)
  {
    View res = frame.findViewById(item.mViewId);
    if (res != null)
    {
      if ((res.getTag() instanceof String) && TAG_COLLAPSE.equals(res.getTag()))
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
    mItemViews.put(item, mapItem(item, mContentFrame));
  }

  private void adjustCollapsedItems()
  {
    for (View v: mCollapseViews)
      v.setAlpha(mCollapsed ? 0.0f : 1.0f);

    if (mAnimationSymmetricalGap == null)
      return;

    LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) mAnimationSymmetricalGap.getLayoutParams();
    lp.width = (mCollapsed ? 0 : mButtonsHeight);
    mAnimationSymmetricalGap.setLayoutParams(lp);
  }

  private void adjustTransparency()
  {
    mFrame.setBackgroundColor(mFrame.getContext().getResources().getColor(isOpen() ? R.color.menu_background_open
                                                                                   : R.color.menu_background_closed));
  }

  private void correctLayout()
  {
    if (mLayoutCorrected)
      return;

    mContentFrame.post(new Runnable()
    {
      @Override
      public void run()
      {
        mContentHeight = mContentFrame.getMeasuredHeight();

        mFrame.removeView(mContentFrame);
        mFrame.addView(mContentFrame);

        if (mOpenDelayed)
          open(false);
        else
          UiUtils.hide(mContentFrame);

        mOpenDelayed = false;
        mLayoutCorrected = true;
      }
    });
  }

  public void updateMarker()
  {
    int count = ActiveCountryTree.getOutOfDateCount();
    UiUtils.showIf(!mCollapsed && (count > 0) && !isOpen(), mNewsMarker);
    UiUtils.showIf(count > 0, mNewsCounter);

    if (count > 0)
      mNewsCounter.setText(String.valueOf(count));
  }

  public void onResume()
  {
    correctLayout();
    updateMarker();
  }

  public void onSaveState(Bundle state)
  {
    if (isOpen())
      state.putBoolean(STATE_OPEN, true);

    state.putString(STATE_LAST_INFO, mCurrentPlace.getText().toString());
  }

  public void onRestoreState(Bundle state)
  {
    mOpenDelayed = (mToggle.mAlwaysShow && state.containsKey(STATE_OPEN));
    updateRoutingInfo(state.getString(STATE_LAST_INFO));
  }

  public void updateRoutingInfo()
  {
    RoutingInfo info = Framework.nativeGetRouteFollowingInfo();

    if (info != null)
      updateRoutingInfo(info.currentStreet);
  }

  private void updateRoutingInfo(String info)
  {
    mCurrentPlace.setText(info);
  }

  private void init()
  {
    mapItem(Item.SEARCH);
    mapItem(Item.ROUTE);
    mapItem(Item.BOOKMARKS);
    mapItem(Item.SHARE);
    mapItem(Item.DOWNLOADER);
    mapItem(Item.SETTINGS);

    adjustCollapsedItems();
    adjustTransparency();
    setNavigationMode(false);
  }

  public MainMenu(ViewGroup frame, Container container)
  {
    mContainer = container;
    mFrame = frame;

    View lineFrame = mFrame.findViewById(R.id.line_frame);
    mButtonsFrame = lineFrame.findViewById(R.id.buttons_frame);
    mNavigationFrame = lineFrame.findViewById(R.id.navigation_frame);
    mContentFrame = mFrame.findViewById(R.id.content_frame);

    mAnimationSpacer = mFrame.findViewById(R.id.animation_spacer);
    mAnimationSymmetricalGap = mButtonsFrame.findViewById(R.id.symmetrical_gap);

    mCurrentPlace = (TextView) mNavigationFrame.findViewById(R.id.current_place);

    mMyPositionButton = new MyPositionButton(lineFrame.findViewById(R.id.my_position));
    mToggle = new Toggle(lineFrame);

    mNewsMarker = mButtonsFrame.findViewById(R.id.marker);
    mNewsCounter = (TextView) mContentFrame.findViewById(R.id.counter);

    init();
  }

  public void setNavigationMode(boolean navigation)
  {
    updateRoutingInfo();
    mToggle.updateNavigationMode(navigation);
    UiUtils.showIf(!navigation, mButtonsFrame);
    UiUtils.showIf(navigation, mNavigationFrame,
                               mItemViews.get(Item.SEARCH),
                               mItemViews.get(Item.BOOKMARKS));
    if (mLayoutCorrected)
    {
      mContentFrame.measure(ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);
      mContentHeight = mContentFrame.getMeasuredHeight();
    }
  }

  public boolean isOpen()
  {
    return (mContentFrame.getVisibility() == View.VISIBLE);
  }

  public boolean shouldOpenDelayed()
  {
    return mOpenDelayed;
  }

  public boolean open(boolean animate)
  {
    if ((animate && mAnimating) || isOpen())
      return false;

    UiUtils.show(mContentFrame);
    adjustCollapsedItems();
    adjustTransparency();
    updateMarker();

    if (!animate)
    {
      mToggle.updateOpenImageNoAnimation(true);
      return true;
    }

    mToggle.animateOpen(true);

    mFrame.setTranslationY(mContentHeight);
    mFrame.animate()
          .setDuration(ANIMATION_DURATION)
          .translationY(0.0f)
          .setListener(new AnimationListener() {})
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

      mToggle.updateOpenImageNoAnimation(false);

      if (onCloseListener != null)
        onCloseListener.run();

      return true;
    }

    mToggle.animateOpen(false);

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
}
