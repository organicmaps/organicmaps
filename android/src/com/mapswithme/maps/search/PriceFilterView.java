package com.mapswithme.maps.search;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.ColorRes;
import android.support.annotation.DrawableRes;
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

import java.util.ArrayList;
import java.util.List;

import static com.mapswithme.maps.search.HotelsFilter.Op.OP_EQ;

public class PriceFilterView extends LinearLayout implements View.OnClickListener
{
  private static final int ONE = 1;
  private static final int TWO = 2;
  private static final int THREE = 3;

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
      int background = UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.clickableBackground);
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
  private SparseArray<Item> mItems = new SparseArray<>();

  public PriceFilterView(Context context)
  {
    this(context, null, 0, 0);
  }

  public PriceFilterView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0, 0);
  }

  public PriceFilterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, 0);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public PriceFilterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    setOrientation(HORIZONTAL);
    LayoutInflater.from(context).inflate(R.layout.price_filter, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    View one = findViewById(R.id.one);
    one.setOnClickListener(this);
    mItems.append(R.id.one, new Item(one, (TextView) findViewById(R.id.one_title)));
    View two = findViewById(R.id.two);
    two.setOnClickListener(this);
    mItems.append(R.id.two, new Item(two, (TextView) findViewById(R.id.two_title)));
    View three = findViewById(R.id.three);
    three.setOnClickListener(this);
    mItems.append(R.id.three, new Item(three, (TextView) findViewById(R.id.three_title)));
  }

  public void update(@Nullable HotelsFilter filter)
  {
    mFilter = filter;
    if (mFilter == null)
    {
      deselectAll();
      return;
    }

    if (mFilter instanceof HotelsFilter.PriceRateFilter)
    {
      HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) mFilter;
      selectByValue(price.mValue);
      return;
    }

    if (!(mFilter instanceof HotelsFilter.Or))
      return;

    HotelsFilter.Or or = (HotelsFilter.Or) mFilter;
    if (or.mLhs instanceof HotelsFilter.PriceRateFilter)
    {
      HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) or.mLhs;
      selectByValue(price.mValue);
    }
    else
    {
      return;
    }

    if (or.mRhs instanceof HotelsFilter.PriceRateFilter)
    {
      HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) or.mRhs;
      selectByValue(price.mValue);
    }
    else if (or.mRhs instanceof HotelsFilter.Or)
    {
      or = (HotelsFilter.Or) or.mRhs;
      if (or.mLhs instanceof HotelsFilter.PriceRateFilter)
      {
        HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) or.mLhs;
        selectByValue(price.mValue);
      }
      if (or.mRhs instanceof HotelsFilter.PriceRateFilter)
      {
        HotelsFilter.PriceRateFilter price = (HotelsFilter.PriceRateFilter) or.mRhs;
        selectByValue(price.mValue);
      }
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

  private void selectByValue(int value)
  {
    switch (value)
    {
      case ONE:
        select(R.id.one, true);
        break;
      case TWO:
        select(R.id.two, true);
        break;
      case THREE:
        select(R.id.three, true);
        break;
    }
  }

  private void select(int id, boolean force)
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      int key = mItems.keyAt(i);
      Item item = mItems.valueAt(i);
      if (key == id)
      {
        if (!force)
          item.select(!item.mSelected);
        else
          item.select(true);
        return;
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    select(v.getId(), false);
    updateFilter();
  }

  private void updateFilter()
  {
    List<HotelsFilter.PriceRateFilter> filters = new ArrayList<>();
    for (int i = 0; i < mItems.size(); ++i)
    {
      int key = mItems.keyAt(i);
      Item item = mItems.valueAt(i);
      if (item.mSelected)
      {
        int value = ONE;
        switch (key)
        {
          case R.id.one:
            value = ONE;
            break;
          case R.id.two:
            value = TWO;
            break;
          case R.id.three:
            value = THREE;
            break;
        }
        filters.add(new HotelsFilter.PriceRateFilter(OP_EQ, value));
      }
    }

    if (filters.isEmpty())
    {
      mFilter = null;
      return;
    }

    if (filters.size() == 1)
    {
      mFilter = filters.get(0);
      return;
    }

    if (filters.size() > 2)
      mFilter =
          new HotelsFilter.Or(filters.get(0), new HotelsFilter.Or(filters.get(1), filters.get(2)));
    else
      mFilter = new HotelsFilter.Or(filters.get(0), filters.get(1));
  }

  @Nullable
  public HotelsFilter getFilter()
  {
    return mFilter;
  }
}
