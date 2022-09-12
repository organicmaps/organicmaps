package com.mapswithme.maps.maplayer;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;

public class SearchWheel implements View.OnClickListener
{
  private static final String EXTRA_CURRENT_OPTION = "extra_current_option";
  private final View mFrame;

  private View mSearchLayout;
  private final ImageView mSearchButton;
  @Nullable
  private final View mTouchInterceptor;

  private boolean mIsExpanded;
  @Nullable
  private SearchOption mCurrentOption;
  @NonNull
  private final View.OnClickListener mOnSearchPressedListener;
  @NonNull
  private final View.OnClickListener mOnSearchCanceledListener;

  private static final long CLOSE_DELAY_MILLIS = 5000L;
  private final Runnable mCloseRunnable = new Runnable() {
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
    FUEL(R.id.search_fuel, R.drawable.ic_routing_fuel_off, R.string.fuel),
    PARKING(R.id.search_parking, R.drawable.ic_routing_parking_off, R.string.parking),
    EAT(R.id.search_eat, R.drawable.ic_routing_eat_off, R.string.eat),
    FOOD(R.id.search_food, R.drawable.ic_routing_food_off, R.string.food),
    ATM(R.id.search_atm, R.drawable.ic_routing_atm_off, R.string.atm);

    @IdRes
    private final int mResId;
    @DrawableRes
    private final int mDrawableOff;
    @StringRes
    private final int mQueryId;

    SearchOption(@IdRes int resId, @DrawableRes int drawableOff,
                 @StringRes int queryId)
    {
      this.mResId = resId;
      this.mDrawableOff = drawableOff;
      this.mQueryId = queryId;
    }

    @NonNull
    public static SearchOption fromResId(@IdRes int resId)
    {
      for (SearchOption searchOption : SearchOption.values())
      {
        if (searchOption.mResId == resId)
          return searchOption;
      }
      throw new IllegalArgumentException("No navigation search for id " + resId);
    }

    @Nullable
    public static SearchOption fromSearchQuery(@NonNull String query, @NonNull Context context)
    {
      final String normalizedQuery = query.trim().toLowerCase();
      for (SearchOption searchOption : SearchOption.values())
      {
        final String searchOptionQuery = context.getString(searchOption.mQueryId).trim().toLowerCase();
        if (searchOptionQuery.equals(normalizedQuery))
          return searchOption;
      }
      return null;
    }
  }

  public SearchWheel(View frame, @NonNull View.OnClickListener onSearchPressedListener, @NonNull View.OnClickListener onSearchCanceledListener)
  {
    mFrame = frame;
    mOnSearchPressedListener = onSearchPressedListener;
    mOnSearchCanceledListener = onSearchCanceledListener;
    mTouchInterceptor = mFrame.findViewById(R.id.touch_interceptor);
    if (mTouchInterceptor != null)
      mTouchInterceptor.setOnClickListener(this);
    mSearchButton = mFrame.findViewById(R.id.btn_search);
    mSearchButton.setOnClickListener(this);
    refreshSearchVisibility();
  }

  private boolean initSearchLayout()
  {
    if (mSearchLayout != null)
      return true;

    mSearchLayout = mFrame.findViewById(R.id.search_frame);
    if (mSearchLayout == null)
      return false;

    DisplayMetrics displayMetrics = new DisplayMetrics();
    WindowManager windowmanager = (WindowManager) mFrame.getContext().getSystemService(Context.WINDOW_SERVICE);
    windowmanager.getDefaultDisplay().getMetrics(displayMetrics);
    // Get available screen height in DP
    int height =  Math.round(displayMetrics.heightPixels / displayMetrics.density);
    // If height is less than 400dp, the search wheel in a straight line
    // In this case, move the pivot for the animation
    if (height < 400)
    {
      UiUtils.waitLayout(mSearchLayout, () -> {
        mSearchLayout.setPivotX(0);
        mSearchLayout.setPivotY(mSearchLayout.getMeasuredHeight() / 2f);
      });
    }
    for (SearchOption searchOption : SearchOption.values())
      mFrame.findViewById(searchOption.mResId).setOnClickListener(this);
    return true;
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mSearchButton);
    if (initSearchLayout())
      UiUtils.showIf(show && mIsExpanded, mSearchLayout);
  }

  public void saveState(@NonNull Bundle outState)
  {
    outState.putSerializable(EXTRA_CURRENT_OPTION, mCurrentOption);
  }

  public void restoreState(@NonNull Bundle savedState)
  {
    mCurrentOption = (SearchOption) savedState.getSerializable(EXTRA_CURRENT_OPTION);
  }

  public void reset()
  {
    mIsExpanded = false;
    mCurrentOption = null;
    resetSearchButtonImage();
  }

  public void onResume()
  {
    if (mCurrentOption != null)
    {
      refreshSearchButtonImage();
      return;
    }

    final String query = SearchEngine.INSTANCE.getQuery();
    if (TextUtils.isEmpty(query))
    {
      resetSearchButtonImage();
      return;
    }

    mCurrentOption = SearchOption.fromSearchQuery(query, mFrame.getContext());
    refreshSearchButtonImage();
  }

  private void toggleSearchLayout()
  {
    if (initSearchLayout())
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
      if (mTouchInterceptor != null)
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
  }

  private void refreshSearchVisibility()
  {
    if (initSearchLayout())
    {
      for (SearchOption searchOption : SearchOption.values())
        UiUtils.visibleIf(mIsExpanded, mSearchLayout.findViewById(searchOption.mResId));

      if (mTouchInterceptor != null)
        UiUtils.visibleIf(mIsExpanded, mSearchLayout, mTouchInterceptor);

      if (mIsExpanded)
      {
        UiThread.cancelDelayedTasks(mCloseRunnable);
        UiThread.runLater(mCloseRunnable, CLOSE_DELAY_MILLIS);
      }
    }
  }

  private void resetSearchButtonImage()
  {
    mSearchButton.setImageDrawable(Graphics.tint(mSearchButton.getContext(),
                                                 R.drawable.ic_search));
  }

  private void refreshSearchButtonImage()
  {
    mSearchButton.setImageDrawable(Graphics.tint(mSearchButton.getContext(),
                                                 mCurrentOption == null ?
                                                 R.drawable.ic_routing_search_off :
                                                 mCurrentOption.mDrawableOff,
                                                 R.attr.colorAccent));
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_search:
      if (!RoutingController.get().isNavigating())
      {
        if (TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()))
          showSearchInParent();
        else
          mOnSearchCanceledListener.onClick(v);
        return;
      }

      if (mCurrentOption != null || !TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()))
      {
        mOnSearchCanceledListener.onClick(v);
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
    case R.id.search_eat:
    case R.id.search_food:
    case R.id.search_atm:
      startSearch(SearchOption.fromResId(v.getId()));
    }
  }

  private void showSearchInParent()
  {
    mOnSearchPressedListener.onClick(mSearchButton);
    mIsExpanded = false;
    refreshSearchVisibility();
  }

  private void startSearch(SearchOption searchOption)
  {
    mCurrentOption = searchOption;
    final String query = mFrame.getContext().getString(searchOption.mQueryId);
    // Category request from navigation search wheel.
    SearchEngine.INSTANCE.searchInteractive(mFrame.getContext(), query, true, System.nanoTime(), false);
    SearchEngine.INSTANCE.setQuery(query);
    refreshSearchButtonImage();
    toggleSearchLayout();
  }
}
