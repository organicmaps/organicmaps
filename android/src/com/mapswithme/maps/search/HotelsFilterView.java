package com.mapswithme.maps.search;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

import com.mapswithme.maps.R;
import com.mapswithme.util.Animations;

public class HotelsFilterView extends FrameLayout
{
  public interface HotelsFilterListener
  {
    void onCancel();

    void onDone(@Nullable HotelsFilter filter);
  }

  private View mFrame;
  private View mFade;
  private RatingFilterView mRating;
  private PriceFilterView mPrice;

  @Nullable
  private HotelsFilterListener mListener;
  @Nullable
  private HotelsFilter mFilter;

  private boolean mOpened = false;

  public HotelsFilterView(Context context)
  {
    this(context, null, 0, 0);
  }

  public HotelsFilterView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0, 0);
  }

  public HotelsFilterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, 0);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public HotelsFilterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
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
    findViewById(R.id.cancel).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mListener != null)
          mListener.onCancel();
        close();
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
  }

  private void populateFilter()
  {
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
  }

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
        if (and.mLhs instanceof HotelsFilter.RatingFilter)
        {
          rating = (HotelsFilter.RatingFilter) and.mLhs;
          price = ((HotelsFilter.And) mFilter).mRhs;
        }
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
}
