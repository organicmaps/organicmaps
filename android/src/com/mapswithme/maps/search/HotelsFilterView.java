package com.mapswithme.maps.search;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ScrollView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Animations;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.UiUtils;

public class HotelsFilterView extends FrameLayout
{
  private static final String STATE_OPENED = "state_opened";

  public interface HotelsFilterListener
  {
    void onCancel();

    void onDone(@Nullable HotelsFilter filter);
  }

  private View mFrame;
  private View mFade;
  private RatingFilterView mRating;
  private PriceFilterView mPrice;
  private View mContent;
  private View mElevation;
  private int mHeaderHeight;
  private int mButtonsHeight;

  @Nullable
  private HotelsFilterListener mListener;
  @Nullable
  private HotelsFilter mFilter;

  private boolean mOpened = false;

  public HotelsFilterView(Context context)
  {
    this(context, null, 0);
  }

  public HotelsFilterView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public HotelsFilterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public HotelsFilterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(context);
  }

  private void init(Context context)
  {
    Resources res = context.getResources();
    mHeaderHeight = (int) res.getDimension(
        UiUtils.getStyledResourceId(context, android.R.attr.actionBarSize));
    mButtonsHeight = (int) res.getDimension(R.dimen.height_block_base);
    LayoutInflater.from(context).inflate(R.layout.hotels_filter, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    mFrame = findViewById(R.id.frame);
    mFrame.setTranslationY(mFrame.getResources().getDisplayMetrics().heightPixels);
    mFade = findViewById(R.id.fade);
    mRating = (RatingFilterView) findViewById(R.id.rating);
    mPrice = (PriceFilterView) findViewById(R.id.price);
    mContent = mFrame.findViewById(R.id.content);
    mElevation = mFrame.findViewById(R.id.elevation);
    findViewById(R.id.cancel).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        cancel();
      }
    });

    findViewById(R.id.done).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        populateFilter();
        if (mListener != null)
          mListener.onDone(mFilter);
        close();
      }
    });
    findViewById(R.id.reset).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mFilter = null;
        if (mListener != null)
          mListener.onDone(null);
        close();
      }
    });
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);

    mContent.measure(widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
    mElevation.measure(widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
    int height = mContent.getMeasuredHeight() + mHeaderHeight + mButtonsHeight
                 + mElevation.getMeasuredHeight();
    if (height >= getMeasuredHeight())
      height = LayoutParams.WRAP_CONTENT;

    ViewGroup.LayoutParams lp = mFrame.getLayoutParams();
    lp.height = height;
    mFrame.setLayoutParams(lp);
  }

  private void cancel()
  {
    updateViews();
    if (mListener != null)
      mListener.onCancel();
    close();
  }

  private void populateFilter()
  {
    mPrice.updateFilter();
    HotelsFilter.RatingFilter rating = mRating.getFilter();
    HotelsFilter price = mPrice.getFilter();
    if (rating == null && price == null)
    {
      mFilter = null;
      return;
    }

    if (rating == null)
    {
      mFilter = price;
      return;
    }

    if (price == null)
    {
      mFilter = rating;
      return;
    }

    mFilter = new HotelsFilter.And(rating, price);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    if (mOpened && !UiUtils.isViewTouched(event, mFrame))
    {
      cancel();
      return true;
    }

    super.onTouchEvent(event);
    return mOpened;
  }

  public boolean close()
  {
    if (!mOpened)
      return false;

    mOpened = false;
    Animations.fadeOutView(mFade, null);
    Animations.disappearSliding(mFrame, Animations.BOTTOM, null);

    return true;
  }

  public void open(@Nullable HotelsFilter filter)
  {
    if (mOpened)
      return;

    mOpened = true;
    mFilter = filter;
    updateViews();
    Animations.fadeInView(mFade, null);
    Animations.appearSliding(mFrame, Animations.BOTTOM, null);
    InputUtils.hideKeyboard(this);
  }

  /**
   * Update views state according with current {@link #mFilter}
   *
   * mFilter may be null or {@link HotelsFilter.RatingFilter} or {@link HotelsFilter.PriceRateFilter}
   * or {@link HotelsFilter.And} or {@link HotelsFilter.Or}.
   *
   * if mFilter is {@link HotelsFilter.And} then mLhs must be {@link HotelsFilter.RatingFilter} and
   * mRhs must be {@link HotelsFilter.PriceRateFilter} or {@link HotelsFilter.Or} with mLhs and mRhs -
   * {@link HotelsFilter.PriceRateFilter}
   *
   * if mFilter is {@link HotelsFilter.Or} then mLhs and mRhs must be {@link HotelsFilter.PriceRateFilter}
   */
  private void updateViews()
  {
    if (mFilter == null)
    {
      mRating.update(null);
      mPrice.update(null);
    }
    else
    {
      HotelsFilter.RatingFilter rating = null;
      HotelsFilter price = null;
      if (mFilter instanceof HotelsFilter.RatingFilter)
      {
        rating = (HotelsFilter.RatingFilter) mFilter;
      }
      else if (mFilter instanceof HotelsFilter.PriceRateFilter)
      {
        price = mFilter;
      }
      else if (mFilter instanceof HotelsFilter.And)
      {
        HotelsFilter.And and = (HotelsFilter.And) mFilter;
        if (!(and.mLhs instanceof HotelsFilter.RatingFilter))
          throw new AssertionError("And.mLhs must be RatingFilter");

        rating = (HotelsFilter.RatingFilter) and.mLhs;
        price = and.mRhs;
      }
      else if (mFilter instanceof HotelsFilter.Or)
      {
        price = mFilter;
      }
      mRating.update(rating);
      mPrice.update(price);
    }
  }

  public void setListener(@Nullable HotelsFilterListener listener)
  {
    mListener = listener;
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_OPENED, mOpened);
  }

  public void onRestoreState(@NonNull Bundle state, @Nullable HotelsFilter filter)
  {
    if (state.getBoolean(STATE_OPENED, false))
      open(filter);
  }
}
