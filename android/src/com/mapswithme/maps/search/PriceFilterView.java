package com.mapswithme.maps.search;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.ColorRes;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

import static com.mapswithme.maps.search.HotelsFilter.Op.OP_EQ;

public class PriceFilterView extends LinearLayout implements View.OnClickListener
{
  private static final int LOW = 1;
  private static final int MEDIUM = 2;
  private static final int HIGH = 3;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ LOW, MEDIUM, HIGH })
  public @interface PriceDef
  {
  }

  private static class Item
  {
    @NonNull
    final View mFrame;
    @NonNull
    final TextView mTitle;
    boolean mSelected;

    private Item(@NonNull View frame, @NonNull TextView title)
    {
      mFrame = frame;
      mTitle = title;
    }

    void select(boolean select)
    {
      mSelected = select;
      update();
    }

    void update()
    {
      @DrawableRes
      int background = UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.filterPropertyBackground);
      @ColorRes
      int titleColor =
          mSelected ? UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)
                 : UiUtils.getStyledResourceId(mFrame.getContext(), android.R.attr.textColorPrimary);
      if (!mSelected)
        mFrame.setBackgroundResource(background);
      else
        mFrame.setBackgroundColor(ContextCompat.getColor(mFrame.getContext(),
                                                         UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
      mTitle.setTextColor(ContextCompat.getColor(mFrame.getContext(), titleColor));
    }
  }

  @Nullable
  private HotelsFilter mFilter;

  @NonNull
  private final SparseArray<Item> mItems = new SparseArray<>();

  public PriceFilterView(Context context)
  {
    this(context, null, 0);
  }

  public PriceFilterView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public PriceFilterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public PriceFilterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(context);
  }

  private void init(Context context)
  {
    setOrientation(HORIZONTAL);
    LayoutInflater.from(context).inflate(R.layout.price_filter, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    View low = findViewById(R.id.low);
    low.setOnClickListener(this);
    mItems.append(R.id.low, new Item(low, (TextView) findViewById(R.id.low_title)));
    View medium = findViewById(R.id.medium);
    medium.setOnClickListener(this);
    mItems.append(R.id.medium, new Item(medium, (TextView) findViewById(R.id.medium_title)));
    View high = findViewById(R.id.high);
    high.setOnClickListener(this);
    mItems.append(R.id.high, new Item(high, (TextView) findViewById(R.id.high_title)));
  }

  public void update(@Nullable HotelsFilter filter)
  {
    mFilter = filter;

    deselectAll();
    if (mFilter != null)
      updateRecursive(mFilter);
  }

  private void updateRecursive(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.PriceRateFilter)
    {
      HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) filter;
      selectByValue(price.mValue);
    }
    else if (filter instanceof HotelsFilter.Or)
    {
      HotelsFilter.Or or = (HotelsFilter.Or) filter;
      updateRecursive(or.mLhs);
      updateRecursive(or.mRhs);
    }
    else
    {
      throw new AssertionError("Wrong hotels filter type");
    }
  }

  private void deselectAll()
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      Item item = mItems.valueAt(i);
      item.select(false);
    }
  }

  private void selectByValue(@PriceDef int value)
  {
    switch (value)
    {
      case LOW:
        select(R.id.low, true);
        break;
      case MEDIUM:
        select(R.id.medium, true);
        break;
      case HIGH:
        select(R.id.high, true);
        break;
    }
  }

  private void select(@IdRes int id, boolean force)
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      int key = mItems.keyAt(i);
      Item item = mItems.valueAt(i);
      if (key == id)
      {
        item.select(force || !item.mSelected);
        return;
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    select(v.getId(), false);
  }

  public void updateFilter()
  {
    List<HotelsFilter.PriceRateFilter> filters = new ArrayList<>();
    for (int i = 0; i < mItems.size(); ++i)
    {
      int key = mItems.keyAt(i);
      Item item = mItems.valueAt(i);
      if (item.mSelected)
      {
        @PriceDef
        int value = LOW;
        switch (key)
        {
          case R.id.low:
            value = LOW;
            break;
          case R.id.medium:
            value = MEDIUM;
            break;
          case R.id.high:
            value = HIGH;
            break;
        }
        filters.add(new HotelsFilter.PriceRateFilter(OP_EQ, value));
      }
    }

    if (filters.size() > 3)
      throw new AssertionError("Wrong filters count");
    mFilter = null;
    for (HotelsFilter filter : filters)
    {
      if (mFilter == null)
        mFilter = filter;
      else
        mFilter = new HotelsFilter.Or(mFilter, filter);
    }
  }

  @Nullable
  public HotelsFilter getFilter()
  {
    return mFilter;
  }
}
