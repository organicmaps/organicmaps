package com.mapswithme.maps.search;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.TagLayoutManager;
import com.mapswithme.util.Animations;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.UiUtils;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class HotelsFilterView extends FrameLayout
    implements HotelsTypeAdapter.OnTypeSelectedListener
{
  private static final String STATE_OPENED = "state_opened";

  interface HotelsFilterListener
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
  private Drawable mTagsDecorator;
  @NonNull
  private final Set<HotelsFilter.HotelType> mHotelTypes = new HashSet<>();
  @Nullable
  private HotelsTypeAdapter mTypeAdapter;

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
    mTagsDecorator = ContextCompat.getDrawable(context, R.drawable.divider_transparent_half);
  }

  @CallSuper
  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();
    if (isInEditMode())
      return;

    mFrame = findViewById(R.id.frame);
    mFrame.setTranslationY(mFrame.getResources().getDisplayMetrics().heightPixels);
    mFade = findViewById(R.id.fade);
    mRating = (RatingFilterView) findViewById(R.id.rating);
    mPrice = (PriceFilterView) findViewById(R.id.price);
    mContent = mFrame.findViewById(R.id.content);
    mElevation = mFrame.findViewById(R.id.elevation);
    RecyclerView type = (RecyclerView) mContent.findViewById(R.id.type);
    type.setLayoutManager(new TagLayoutManager());
    type.setNestedScrollingEnabled(false);
    type.addItemDecoration(new TagItemDecoration(mTagsDecorator));
    mTypeAdapter = new HotelsTypeAdapter(this);
    type.setAdapter(mTypeAdapter);
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
        mHotelTypes.clear();
        if (mListener != null)
          mListener.onDone(null);
        updateViews();
      }
    });
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);

    if (isInEditMode())
      return;

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
    if (rating == null && price == null && mHotelTypes.isEmpty())
    {
      mFilter = null;
      return;
    }

    final HotelsFilter.OneOf oneOf = makeOneOf(mHotelTypes.iterator());

    if (rating == null)
    {
      if (oneOf != null && price != null)
      {
        mFilter = new HotelsFilter.And(price, oneOf);
        return;
      }
      else if (price == null && oneOf != null)
      {
        mFilter = oneOf;
        return;
      }
      else if (price != null)
      {
        mFilter = price;
        return;
      }
      else
      {
        mFilter = null;
        return;
      }
    }

    if (price == null)
    {
      if (oneOf != null)
      {
        mFilter = new HotelsFilter.And(rating, oneOf);
        return;
      }
      mFilter = rating;
      return;
    }

    if (oneOf != null)
    {
      mFilter = new HotelsFilter.And(rating, new HotelsFilter.And(price, oneOf));
      return;
    }

    mFilter = new HotelsFilter.And(rating, price);
  }

  @Nullable
  private HotelsFilter.OneOf makeOneOf(Iterator<HotelsFilter.HotelType> iterator)
  {
    if (!iterator.hasNext())
      return null;

    HotelsFilter.HotelType type = iterator.next();
    return new HotelsFilter.OneOf(type, makeOneOf(iterator));
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
    Animations.fadeInView(mFade, null);
    Animations.appearSliding(mFrame, Animations.BOTTOM, new Runnable()
    {
      @Override
      public void run()
      {
        updateViews();
      }
    });
    InputUtils.hideKeyboard(this);
  }

  /**
   * Update views state according with current {@link #mFilter}
   *
   * mFilter may be null or {@link HotelsFilter.RatingFilter} or {@link HotelsFilter.PriceRateFilter}
   * or {@link HotelsFilter.And} or {@link HotelsFilter.Or} or {@link HotelsFilter.OneOf}.
   *
   * if mHotelTypes is not empty then mFilter must be {@link HotelsFilter.OneOf} or
   * {@link HotelsFilter.And} with mLhs - ({@link HotelsFilter.PriceRateFilter} or
   * {@link HotelsFilter.Or} or {@link HotelsFilter.RatingFilter}) and (mRhs - {@link HotelsFilter.OneOf})
   *
   * if mFilter is {@link HotelsFilter.And} and mHotelTypes is empty then mLhs must be
   * {@link HotelsFilter.RatingFilter} and mRhs must be {@link HotelsFilter.PriceRateFilter} or
   * {@link HotelsFilter.Or} with mLhs and mRhs - {@link HotelsFilter.PriceRateFilter}
   *
   * if mFilter is {@link HotelsFilter.Or} then mLhs and mRhs must be {@link HotelsFilter.PriceRateFilter}
   */
  private void updateViews()
  {
    if (mFilter == null)
    {
      mRating.update(null);
      mPrice.update(null);
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, null);
    }
    else
    {
      HotelsFilter.RatingFilter rating = null;
      HotelsFilter price = null;
      HotelsFilter.OneOf types = null;
      if (mFilter instanceof HotelsFilter.RatingFilter)
      {
        rating = (HotelsFilter.RatingFilter) mFilter;
      }
      else if (mFilter instanceof HotelsFilter.PriceRateFilter)
      {
        price = mFilter;
      }
      else if (mFilter instanceof  HotelsFilter.OneOf)
      {
        types = (HotelsFilter.OneOf) mFilter;
      }
      else if (mFilter instanceof HotelsFilter.And)
      {
        HotelsFilter.And and = (HotelsFilter.And) mFilter;
        if (!(and.mLhs instanceof HotelsFilter.RatingFilter)
            && !(and.mLhs instanceof HotelsFilter.PriceRateFilter)
            && !(and.mLhs instanceof HotelsFilter.Or))
        {
          throw new AssertionError("And.mLhs must be RatingFilter or PriceRateFilter or Or");
        }

        if (and.mLhs instanceof HotelsFilter.RatingFilter)
        {
          rating = (HotelsFilter.RatingFilter) and.mLhs;

          if (!(and.mRhs instanceof HotelsFilter.And)
              && !(and.mRhs instanceof HotelsFilter.PriceRateFilter)
              && !(and.mRhs instanceof HotelsFilter.Or))
          {
            throw new AssertionError("And.mRhs must be And or PriceRateFilter or Or");
          }

          if (and.mRhs instanceof HotelsFilter.And)
          {
            HotelsFilter.And rand = (HotelsFilter.And) and.mRhs;
            if (!(rand.mLhs instanceof HotelsFilter.PriceRateFilter)
                && !(rand.mLhs instanceof HotelsFilter.Or))
            {
              throw new AssertionError("And.mLhs must be PriceRateFilter or Or");
            }
            price = rand.mLhs;

            if (!(rand.mRhs instanceof HotelsFilter.OneOf))
              throw new AssertionError("And.mRhs must be OneOf");

            types = (HotelsFilter.OneOf) rand.mRhs;
          }
        }
        else
        {
          price = and.mLhs;
          if (!(and.mRhs instanceof HotelsFilter.OneOf))
            throw new AssertionError("And.mRhs must be OneOf");

          types = (HotelsFilter.OneOf) and.mRhs;
        }
      }
      else if (mFilter instanceof HotelsFilter.Or)
      {
        price = mFilter;
      }
      mRating.update(rating);
      mPrice.update(price);
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, types);
    }
  }

  private void updateTypeAdapter(@NonNull HotelsTypeAdapter typeAdapter,
                                 @Nullable HotelsFilter.OneOf types)
  {
    mHotelTypes.clear();
    if (types != null)
      populateHotelTypes(mHotelTypes, types);
    typeAdapter.updateItems(mHotelTypes);
  }

  private void populateHotelTypes(@NonNull Set<HotelsFilter.HotelType> hotelTypes,
                                  @NonNull HotelsFilter.OneOf types)
  {
    hotelTypes.add(types.mType);
    if (types.mRhs != null)
      populateHotelTypes(hotelTypes, types.mRhs);
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

  @Override
  public void onTypeSelected(boolean selected, @NonNull HotelsFilter.HotelType type)
  {
    if (selected)
      mHotelTypes.add(type);
    else
      mHotelTypes.remove(type);
  }
}
