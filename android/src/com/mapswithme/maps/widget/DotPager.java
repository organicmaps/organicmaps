package com.mapswithme.maps.widget;

import android.content.Context;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

/**
 * This widget allows the user to flip left and right through pages of data.
 * Also it can be configured with indicator view that will be placed below of pages.
 * The indicator will be drawn as the gray dots. A dots count will be equal page count.
 * The bold dot corresponds the current page.
 * <p>
 * There are few dependencies that should be provided to get this
 * widget work:
 * <p>
 * <ul>
 * <li>@see {@link ViewPager}</li>
 * <li>@see {@link PagerAdapter}</li>
 * <li>An indicator. It's a {@link ViewGroup} which will consist dots. If the indicator
 * is not needed this dependency can be missed or <code>null</code></li>
 * <li>@see {@link Context}</li>
 * <li>A page listener is an observable mechanism for listening page changing. It can be missed or null</li>
 * </ul>
 */
public class DotPager implements ViewPager.OnPageChangeListener
{
  @NonNull
  private final ViewPager mPager;
  @NonNull
  private final PagerAdapter mAdapter;
  @Nullable
  private final ViewGroup mIndicator;
  @NonNull
  private final ImageView[] mDots;
  @NonNull
  private final Context mContext;
  @Nullable
  private final OnPageChangedListener mListener;
  @StringRes
  private final int mActiveDotDrawableResId;
  @StringRes
  private final int mInactiveDotDrawableResId;


  private DotPager(@NonNull Builder builder)
  {
    mContext = builder.mContext;
    mPager = builder.mPager;
    mAdapter = builder.mAdapter;
    mIndicator = builder.mIndicatorContainer;
    mListener = builder.mListener;
    mDots = new ImageView[mAdapter.getCount()];
    mActiveDotDrawableResId = builder.mActiveDotDrawableResId;
    mInactiveDotDrawableResId = builder.mInactiveDotDrawableResId;
  }

  public void show()
  {
    configure();
    updateIndicator();
  }

  private void configure()
  {
    configurePager();
    configureIndicator();
  }

  private void configurePager()
  {
    mPager.setAdapter(mAdapter);
    mPager.clearOnPageChangeListeners();
    mPager.addOnPageChangeListener(this);
  }

  private void configureIndicator()
  {
    if (mIndicator == null)
      return;

    mIndicator.removeAllViews();

    if (mAdapter.getCount() == 1)
      return;

    for (int i = 0; i < mDots.length; i++)
    {
      mDots[i] = new ImageView(mContext);
      LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
          ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
      layoutParams.setMargins(0, 0, UiUtils.dimen(mContext, R.dimen.margin_half), 0);
      mIndicator.addView(mDots[i], i, layoutParams);
    }
  }

  @Override
  public void onPageSelected(int position)
  {
    if (mIndicator != null)
      updateIndicator();

    if (mListener != null)
      mListener.onPageChanged(position);
  }

  private void updateIndicator()
  {
    if (mAdapter.getCount() == 1)
      return;

    int currentPage = mPager.getCurrentItem();
    for (int i = 0; i < mAdapter.getCount(); i++)
    {
      boolean isCurPage = i == currentPage;
      @DrawableRes int dotDrawable;
      if (ThemeUtils.isNightTheme(mContext))
        dotDrawable = isCurPage ? R.drawable.news_marker_active_night : R.drawable.news_marker_inactive_night;
      else
        dotDrawable = isCurPage ? mActiveDotDrawableResId : mInactiveDotDrawableResId;
      mDots[i].setImageResource(dotDrawable);
    }
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
  {
    //no op
  }

  @Override
  public void onPageScrollStateChanged(int state)
  {
    //no op
  }

  public static class Builder
  {
    @NonNull
    private final ViewPager mPager;
    @NonNull
    private final PagerAdapter mAdapter;
    @Nullable
    private ViewGroup mIndicatorContainer;
    @NonNull
    private final Context mContext;
    @Nullable
    private OnPageChangedListener mListener;
    @DrawableRes
    private int mActiveDotDrawableResId = R.drawable.news_marker_active;
    @DrawableRes
    private int mInactiveDotDrawableResId = R.drawable.news_marker_inactive;

    public Builder(@NonNull Context context, @NonNull ViewPager pager, @NonNull PagerAdapter adapter)
    {
      mContext = context;
      mPager = pager;
      mAdapter = adapter;
    }

    public Builder setIndicatorContainer(@NonNull ViewGroup indicatorContainer)
    {
      mIndicatorContainer = indicatorContainer;
      return this;
    }

    public Builder setPageChangedListener(@Nullable OnPageChangedListener listener)
    {
      mListener = listener;
      return this;
    }

    @NonNull
    public Builder setActiveDotDrawable(@DrawableRes int resId)
    {
      mActiveDotDrawableResId = resId;
      return this;
    }

    @NonNull
    public Builder setInactiveDotDrawable(@DrawableRes int resId)
    {
      mInactiveDotDrawableResId = resId;
      return this;
    }

    public DotPager build()
    {
      return new DotPager(this);
    }
  }

  public interface OnPageChangedListener
  {
    void onPageChanged(int position);
  }
}
