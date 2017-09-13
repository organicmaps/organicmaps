package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.ImageView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;

import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_SEARCH_CLICK;

class SearchWheel implements View.OnClickListener
{
  private static final String EXTRA_CURRENT_OPTION = "extra_current_option";
  private final View mFrame;

  private final View mSearchLayout;
  private final ImageView mSearchButton;
  private final View mTouchInterceptor;

  private boolean mIsExpanded;
  @Nullable
  private SearchOption mCurrentOption;

  private static final long CLOSE_DELAY_MILLIS = 5000L;
  private Runnable mCloseRunnable = new Runnable() {
    @Override
    public void run()
    {
      // if the search bar is already closed, i.e. nothing should be done here.
      if (!mIsExpanded)
        return;

      toggleSearchLayout();
    }
  };

  private enum SearchOption
  {
    FUEL(R.id.search_fuel, R.drawable.ic_routing_fuel_off, R.drawable.ic_routing_fuel_on, "fuel"),
    PARKING(R.id.search_parking, R.drawable.ic_routing_parking_off, R.drawable.ic_routing_parking_on, "parking"),
    FOOD(R.id.search_food, R.drawable.ic_routing_food_off, R.drawable.ic_routing_food_on, "food"),
    SHOP(R.id.search_shop, R.drawable.ic_routing_shop_off, R.drawable.ic_routing_shop_on, "shop"),
    ATM(R.id.search_atm, R.drawable.ic_routing_atm_off, R.drawable.ic_routing_atm_on, "atm");

    private int mResId;
    private int mDrawableOff;
    private int mDrawableOn;
    private String mSearchQuery;

    SearchOption(@IdRes int resId, @DrawableRes int drawableOff, @DrawableRes int drawableOn, String searchQuery)
    {
      this.mResId = resId;
      this.mDrawableOff = drawableOff;
      this.mDrawableOn = drawableOn;
      this.mSearchQuery = searchQuery;
    }

    @NonNull
    public static SearchOption FromResId(@IdRes int resId)
    {
      for (SearchOption searchOption : SearchOption.values())
      {
        if (searchOption.mResId == resId)
          return searchOption;
      }
      throw new IllegalArgumentException("No navigation search for id " + resId);
    }

    @Nullable
    public static SearchOption FromSearchQuery(@NonNull String query)
    {
      final String normalizedQuery = query.trim().toLowerCase();
      for (SearchOption searchOption : SearchOption.values())
      {
        if (searchOption.mSearchQuery.equals(normalizedQuery))
          return searchOption;
      }
      return null;
    }
  }

  SearchWheel(View frame)
  {
    mFrame = frame;

    mTouchInterceptor = mFrame.findViewById(R.id.touch_interceptor);
    mTouchInterceptor.setOnClickListener(this);
    mSearchButton = (ImageView) mFrame.findViewById(R.id.btn_search);
    mSearchButton.setOnClickListener(this);
    mSearchLayout = mFrame.findViewById(R.id.search_frame);
    if (UiUtils.isLandscape(mFrame.getContext()))
    {
      UiUtils.waitLayout(mSearchLayout, new ViewTreeObserver.OnGlobalLayoutListener()
      {
        @Override
        public void onGlobalLayout()
        {
          mSearchLayout.setPivotX(0);
          mSearchLayout.setPivotY(mSearchLayout.getMeasuredHeight() / 2);
        }
      });
    }
    for (SearchOption searchOption : SearchOption.values())
      mFrame.findViewById(searchOption.mResId).setOnClickListener(this);
    refreshSearchVisibility();
  }

  void saveState(@NonNull Bundle outState)
  {
    outState.putSerializable(EXTRA_CURRENT_OPTION, mCurrentOption);
  }

  void restoreState(@NonNull Bundle savedState)
  {
    mCurrentOption = (SearchOption) savedState.getSerializable(EXTRA_CURRENT_OPTION);
  }

  public void reset()
  {
    mIsExpanded = false;
    mCurrentOption = null;
    SearchEngine.cancelInteractiveSearch();
    resetSearchButtonImage();
  }

  public void onResume()
  {
    if (mCurrentOption != null)
    {
      refreshSearchButtonImage();
      return;
    }

    final String query = SearchEngine.getQuery();
    if (TextUtils.isEmpty(query))
    {
      resetSearchButtonImage();
      return;
    }

    mCurrentOption = SearchOption.FromSearchQuery(query);
    refreshSearchButtonImage();
  }

  private void toggleSearchLayout()
  {
    final int animRes;
    if (mIsExpanded)
    {
      animRes = R.animator.show_zoom_out_alpha;
    }
    else
    {
      animRes = R.animator.show_zoom_in_alpha;
      UiUtils.show(mSearchLayout);
    }
    mIsExpanded = !mIsExpanded;
    final Animator animator = AnimatorInflater.loadAnimator(mSearchLayout.getContext(), animRes);
    animator.setTarget(mSearchLayout);
    animator.start();
    UiUtils.visibleIf(mIsExpanded, mTouchInterceptor);
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        refreshSearchVisibility();
      }
    });
  }

  private void refreshSearchVisibility()
  {
    for (SearchOption searchOption : SearchOption.values())
      UiUtils.visibleIf(mIsExpanded, mSearchLayout.findViewById(searchOption.mResId));

    UiUtils.visibleIf(mIsExpanded, mSearchLayout, mTouchInterceptor);

    if (mIsExpanded)
    {
      UiThread.cancelDelayedTasks(mCloseRunnable);
      UiThread.runLater(mCloseRunnable, CLOSE_DELAY_MILLIS);
    }
  }

  private void resetSearchButtonImage()
  {
    mSearchButton.setImageDrawable(Graphics.tint(mSearchButton.getContext(),
                                                 R.drawable.ic_routing_search_on));
  }

  private void refreshSearchButtonImage()
  {
    mSearchButton.setImageDrawable(Graphics.tint(mSearchButton.getContext(),
                                                 mCurrentOption == null ?
                                                 R.drawable.ic_routing_search_off :
                                                 mCurrentOption.mDrawableOff,
                                                 R.attr.colorAccent));
  }

  public boolean performClick()
  {
    return mSearchButton.performClick();
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_search:
      if (RoutingController.get().isPlanning())
      {
        if (TextUtils.isEmpty(SearchEngine.getQuery()))
        {
          showSearchInParent();
          Statistics.INSTANCE.trackRoutingEvent(ROUTING_SEARCH_CLICK, true);
        }
        else
        {
          reset();
        }
        return;
      }

      Statistics.INSTANCE.trackRoutingEvent(ROUTING_SEARCH_CLICK, false);
      if (mCurrentOption != null || !TextUtils.isEmpty(SearchEngine.getQuery()))
      {
        SearchEngine.cancelInteractiveSearch();
        mCurrentOption = null;
        mIsExpanded = false;
        resetSearchButtonImage();
        refreshSearchVisibility();
        return;
      }

      if (mIsExpanded)
      {
        showSearchInParent();
        return;
      }

      toggleSearchLayout();
      break;
    case R.id.touch_interceptor:
      toggleSearchLayout();
      break;
    case R.id.search_fuel:
    case R.id.search_parking:
    case R.id.search_food:
    case R.id.search_shop:
    case R.id.search_atm:
      startSearch(SearchOption.FromResId(v.getId()));
    }
  }

  private void showSearchInParent()
  {
    Context context = mFrame.getContext();
    final MwmActivity parent;
    if (context instanceof ContextThemeWrapper)
      parent = (MwmActivity)((ContextThemeWrapper)context).getBaseContext();
    else if (context instanceof android.support.v7.view.ContextThemeWrapper)
      parent = (MwmActivity)((android.support.v7.view.ContextThemeWrapper)context).getBaseContext();
    else
      parent = (MwmActivity) context;
    parent.showSearch();
    mIsExpanded = false;
    refreshSearchVisibility();
  }

  private void startSearch(SearchOption searchOption)
  {
    mCurrentOption = searchOption;
    SearchEngine.searchInteractive(searchOption.mSearchQuery, System.nanoTime(), false /* isMapAndTable */, null /* hotelsFilter */);
    refreshSearchButtonImage();

    toggleSearchLayout();
  }
}
