package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.ImageView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

class SearchWheel implements View.OnClickListener
{
  private static String TAG = "TEST";
  private final View mFrame;

  private final View mSearchLayout;
  private final ImageView mSearchButton;

  private boolean mIsExpanded;
  private SearchOption mCurrentOption;

  private enum SearchOption
  {
    FUEL(R.id.search_fuel, R.drawable.ic_routing_fuel_off, R.drawable.ic_routing_fuel_on, "fuel"),
    PARKING(R.id.search_parking, R.drawable.ic_routing_parking_off, R.drawable.ic_routing_parking_on, "parking"),
    FOOD(R.id.search_food, R.drawable.ic_routing_food_off, R.drawable.ic_routing_food_on, "food"),
    SHOP(R.id.search_shop, R.drawable.ic_routing_shop_off, R.drawable.ic_routing_shop_on, "shop"),
    ATM(R.id.search_atm, R.drawable.ic_routing_atm_off, R.drawable.ic_routing_atm_on, "atm");

    private int resId;
    private int drawableOff;
    private int drawableOn;
    private String searchQuery;

    SearchOption(@IdRes int resId, @DrawableRes int drawableOff, @DrawableRes int drawableOn, String searchQuery)
    {
      this.resId = resId;
      this.drawableOff = drawableOff;
      this.drawableOn = drawableOn;
      this.searchQuery = searchQuery;
    }

    @NonNull
    public static SearchOption FromResId(@IdRes int resId)
    {
      for (SearchOption searchOption : SearchOption.values())
      {
        if (searchOption.resId == resId)
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
        if (searchOption.searchQuery.equals(normalizedQuery))
          return searchOption;
      }
      return null;
    }
  }

  SearchWheel(View frame)
  {
    mFrame = frame;

    // Search
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
      mFrame.findViewById(searchOption.resId).setOnClickListener(this);
    refreshSearchVisibility();
  }

  public void reset()
  {
    mIsExpanded = false;
    mCurrentOption = null;
    SearchEngine.cancelSearch();
  }

  public void onResume()
  {
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
      UiUtils.visibleIf(mIsExpanded, mSearchLayout.findViewById(searchOption.resId));

    UiUtils.visibleIf(mIsExpanded, mSearchLayout);
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
                                                 mCurrentOption.drawableOff,
                                                 R.attr.colorAccent));
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_search:
      if (mCurrentOption != null || !TextUtils.isEmpty(SearchEngine.getQuery()))
      {
        SearchEngine.cancelSearch();
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
    default:
      startSearch(SearchOption.FromResId(v.getId()));
    }
  }

  private void showSearchInParent()
  {
    final MwmActivity parent = (MwmActivity) mFrame.getContext();
    parent.showSearch();
    mIsExpanded = false;
    refreshSearchVisibility();
  }

  private void startSearch(SearchOption searchOption)
  {
    mCurrentOption = searchOption;
    SearchEngine.searchInteractive(searchOption.searchQuery, System.nanoTime(), false /* isMapAndTable */);
    refreshSearchButtonImage();

    toggleSearchLayout();
  }
}
