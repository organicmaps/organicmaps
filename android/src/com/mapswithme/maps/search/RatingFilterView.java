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

import static com.mapswithme.maps.search.HotelsFilter.Op.OP_GE;

public class RatingFilterView extends LinearLayout implements View.OnClickListener
{
  private static final float GOOD = 7.0f;
  private static final float VERY_GOOD = 8.0f;
  private static final float EXCELLENT = 9.0f;

  private static class Item
  {
    @NonNull
    final View mFrame;
    @NonNull
    final TextView mTitle;
    @Nullable
    final TextView mSubtitle;

    private Item(@NonNull View frame, @NonNull TextView title, @Nullable TextView subtitle)
    {
      mFrame = frame;
      mTitle = title;
      mSubtitle = subtitle;
    }

    public void select(boolean select)
    {
      @DrawableRes
      int background = UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.filterPropertyBackground);
      @ColorRes
      int titleColor =
          select ? UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)
                 : UiUtils.getStyledResourceId(mFrame.getContext(), android.R.attr.textColorPrimary);
      @ColorRes
      int subtitleColor =
          select ? UiUtils.getStyledResourceId(mFrame.getContext(), android.R.attr.textColorSecondaryInverse)
                 : UiUtils.getStyledResourceId(mFrame.getContext(), android.R.attr.textColorSecondary);
      if (!select)
        mFrame.setBackgroundResource(background);
      else
        mFrame.setBackgroundColor(ContextCompat.getColor(mFrame.getContext(),
            UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
      mTitle.setTextColor(ContextCompat.getColor(mFrame.getContext(), titleColor));
      if (mSubtitle != null)
        mSubtitle.setTextColor(ContextCompat.getColor(mFrame.getContext(), subtitleColor));
    }
  }

  @Nullable
  private HotelsFilter.RatingFilter mFilter;

  @NonNull
  private final SparseArray<Item> mItems = new SparseArray<>();

  public RatingFilterView(Context context)
  {
    this(context, null, 0);
  }

  public RatingFilterView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public RatingFilterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public RatingFilterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(context);
  }

  private void init(Context context)
  {
    setOrientation(HORIZONTAL);
    LayoutInflater.from(context).inflate(R.layout.rating_filter, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    View any = findViewById(R.id.any);
    any.setOnClickListener(this);
    mItems.append(R.id.any, new Item(any, findViewById(R.id.any_title), null));
    View good = findViewById(R.id.good);
    good.setOnClickListener(this);
    mItems.append(R.id.good, new Item(good, findViewById(R.id.good_title),
                                      findViewById(R.id.good_subtitle)));
    View veryGood = findViewById(R.id.very_good);
    veryGood.setOnClickListener(this);
    mItems.append(R.id.very_good, new Item(veryGood, findViewById(R.id.very_good_title),
                                           findViewById(R.id.very_good_subtitle)));
    View excellent = findViewById(R.id.excellent);
    excellent.setOnClickListener(this);
    mItems.append(R.id.excellent, new Item(excellent, findViewById(R.id.excellent_title),
                                           findViewById(R.id.excellent_subtitle)));
  }

  public void update(@Nullable HotelsFilter.RatingFilter filter)
  {
    mFilter = filter;

    if (mFilter == null)
      select(R.id.any);
    else if (mFilter.mValue == GOOD)
      select(R.id.good);
    else if (mFilter.mValue == VERY_GOOD)
      select(R.id.very_good);
    else if (mFilter.mValue == EXCELLENT)
      select(R.id.excellent);
  }

  private void select(int id)
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      int key = mItems.keyAt(i);
      Item item = mItems.valueAt(i);
      item.select(key == id);
    }
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
      case R.id.any:
        update(null);
        break;
      case R.id.good:
        update(new HotelsFilter.RatingFilter(OP_GE, GOOD));
        break;
      case R.id.very_good:
        update(new HotelsFilter.RatingFilter(OP_GE, VERY_GOOD));
        break;
      case R.id.excellent:
        update(new HotelsFilter.RatingFilter(OP_GE, EXCELLENT));
        break;
    }
  }

  @Nullable
  public HotelsFilter.RatingFilter getFilter()
  {
    return mFilter;
  }
}
