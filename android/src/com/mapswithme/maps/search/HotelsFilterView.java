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
    mRating = findViewById(R.id.rating);
    mPrice = findViewById(R.id.price);
    mContent = mFrame.findViewById(R.id.content);
    mElevation = mFrame.findViewById(R.id.elevation);
    RecyclerView type = mContent.findViewById(R.id.type);
    type.setLayoutManager(new TagLayoutManager());
    type.setNestedScrollingEnabled(false);
    type.addItemDecoration(new TagItemDecoration(mTagsDecorator));
    mTypeAdapter = new HotelsTypeAdapter(this);
    type.setAdapter(mTypeAdapter);
    findViewById(R.id.cancel).setOnClickListener(v -> cancel());

    findViewById(R.id.done).setOnClickListener(v ->
                                               {
                                                 populateFilter();
                                                 if (mListener != null)
                                                   mListener.onDone(mFilter);
                                                 close();
                                               });
    findViewById(R.id.reset).setOnClickListener(v ->
                                                {
                                                  mFilter = null;
                                                  mHotelTypes.clear();
                                                  if (mListener != null)
                                                    mListener.onDone(null);
                                                  updateViews();
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
    final HotelsFilter.RatingFilter rating = mRating.getFilter();
    final HotelsFilter price = mPrice.getFilter();
    final HotelsFilter.OneOf oneOf = makeOneOf(mHotelTypes.iterator());

    mFilter = combineFilters(rating, price, oneOf);
  }

  @Nullable
  private HotelsFilter.OneOf makeOneOf(@NonNull Iterator<HotelsFilter.HotelType> iterator)
  {
    if (!iterator.hasNext())
      return null;

    HotelsFilter.HotelType type = iterator.next();
    return new HotelsFilter.OneOf(type, makeOneOf(iterator));
  }

  @Nullable
  private HotelsFilter combineFilters(@NonNull HotelsFilter... filters)
  {
    HotelsFilter result = null;
    for (HotelsFilter filter : filters)
    {
      if (result == null)
      {
        result = filter;
        continue;
      }

      if (filter != null)
        result = new HotelsFilter.And(filter, result);
    }

    return result;
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
      mRating.update(findRatingFilter(mFilter));
      mPrice.update(findPriceFilter(mFilter));
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, findTypeFilter(mFilter));
    }
  }

  @Nullable
  private HotelsFilter.RatingFilter findRatingFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.RatingFilter)
      return (HotelsFilter.RatingFilter) filter;

    HotelsFilter.RatingFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findRatingFilter(and.mLhs);
      if (result == null)
        result = findRatingFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  private HotelsFilter findPriceFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.PriceRateFilter)
      return filter;

    if (filter instanceof HotelsFilter.Or)
    {
      HotelsFilter.Or or = (HotelsFilter.Or) filter;
      if (or.mLhs instanceof HotelsFilter.PriceRateFilter
          && or.mRhs instanceof HotelsFilter.PriceRateFilter )
      {
        return filter;
      }
    }

    HotelsFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findPriceFilter(and.mLhs);
      if (result == null)
        result = findPriceFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  private HotelsFilter.OneOf findTypeFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.OneOf)
      return (HotelsFilter.OneOf) filter;

    HotelsFilter.OneOf result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findTypeFilter(and.mLhs);
      if (result == null)
        result = findTypeFilter(and.mRhs);

      return result;
    }

    return null;
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
    if (types.mTile != null)
      populateHotelTypes(hotelTypes, types.mTile);
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
