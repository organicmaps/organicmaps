package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class SearchFilterController
{
  private static final String STATE_HOTEL_FILTER = "state_hotel_filter";
  private static final String STATE_HOTEL_FILTER_VISIBILITY = "state_hotel_filter_visibility";

  @NonNull
  private final View mFrame;
  @NonNull
  private final TextView mShowOnMap;
  @NonNull
  private final View mFilterButton;
  @NonNull
  private final ImageView mFilterIcon;
  @NonNull
  private final TextView mFilterText;
  @NonNull
  private final View mDivider;
  @NonNull
  private final HotelsFilterView mFilterView;

  @Nullable
  private HotelsFilter mFilter;

  @Nullable
  private BookingFilterParams mBookingFilterParams;

  private final float mElevation;

  @NonNull
  private final View.OnClickListener mClearListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      setFilter(null);
      if (mFilterListener != null)
        mFilterListener.onFilterClear();
    }
  };

  @Nullable
  private final FilterListener mFilterListener;

  interface FilterListener
  {
    void onViewClick();
    void onFilterClick();
    void onFilterClear();
    void onFilterCancel();
    void onFilterDone();
  }

  SearchFilterController(@NonNull View frame, @NonNull HotelsFilterView filter,
                         @Nullable FilterListener listener)
  {
    this(frame, filter, listener, R.string.search_show_on_map);
  }

  public SearchFilterController(@NonNull View frame, @NonNull HotelsFilterView filter,
                                @Nullable FilterListener listener,
                                @StringRes int populateButtonText)
  {
    mFrame = frame;
    mFilterView = filter;
    mFilterListener = listener;
    mShowOnMap = (TextView) mFrame.findViewById(R.id.show_on_map);
    mShowOnMap.setText(populateButtonText);
    mFilterButton = mFrame.findViewById(R.id.filter_button);
    mFilterIcon = (ImageView) mFilterButton.findViewById(R.id.filter_icon);
    mFilterText = (TextView) mFilterButton.findViewById(R.id.filter_text);
    mDivider = mFrame.findViewById(R.id.divider);

    Resources res = mFrame.getResources();
    mElevation = res.getDimension(R.dimen.margin_quarter);

    initListeners();
  }

  public void show(boolean show, boolean showPopulateButton)
  {
    UiUtils.showIf(show, mFrame);
    showPopulateButton(showPopulateButton);
  }

  void showPopulateButton(boolean show)
  {
    UiUtils.showIf(show, mShowOnMap);
  }

  void showDivider(boolean show)
  {
    UiUtils.showIf(show, mDivider);
  }

  public void updateFilterButtonVisibility(boolean isHotel)
  {
    UiUtils.showIf(isHotel, mFilterButton);
  }

  private void initListeners()
  {
    mShowOnMap.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mFilterListener != null)
          mFilterListener.onViewClick();
      }
    });
    mFilterButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mFilterView.open(getFilter());
        if (mFilterListener != null)
          mFilterListener.onFilterClick();
      }
    });
    mFilterView.setListener(new HotelsFilterView.HotelsFilterListener()
    {
      @Override
      public void onCancel()
      {
        if (mFilterListener != null)
          mFilterListener.onFilterCancel();
      }

      @Override
      public void onDone(@Nullable HotelsFilter filter)
      {
        setFilter(filter);

        if (mFilterListener != null)
          mFilterListener.onFilterDone();
      }
    });
  }

  @Nullable
  public HotelsFilter getFilter()
  {
    return mFilter;
  }

  public void setFilter(@Nullable HotelsFilter filter)
  {
    mFilter = filter;
    if (mFilter != null)
    {
      mFilterIcon.setOnClickListener(mClearListener);
      mFilterIcon.setImageResource(R.drawable.ic_cancel);
      mFilterIcon.setColorFilter(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)));
      UiUtils.setBackgroundDrawable(mFilterButton, R.attr.accentButtonRoundBackground);
      mFilterText.setTextColor(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)));
    }
    else
    {
      mFilterIcon.setOnClickListener(null);
      mFilterIcon.setImageResource(R.drawable.ic_filter_list);
      mFilterIcon.setColorFilter(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
      UiUtils.setBackgroundDrawable(mFilterButton, R.attr.clickableBackground);
      mFilterText.setTextColor(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
    }
  }

  public void resetFilter()
  {
    setFilter(null);
    updateFilterButtonVisibility(false);
  }

  @Nullable
  public BookingFilterParams getBookingFilterParams()
  {
    return mBookingFilterParams;
  }

  public void setBookingFilterParams(@Nullable BookingFilterParams params)
  {
    mBookingFilterParams = params;
  }

  public boolean onBackPressed()
  {
    return mFilterView.close();
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putParcelable(STATE_HOTEL_FILTER, mFilter);
    outState.putBoolean(STATE_HOTEL_FILTER_VISIBILITY,
                        mFilterButton.getVisibility() == View.VISIBLE);
    mFilterView.onSaveState(outState);
  }

  public void onRestoreState(@NonNull Bundle state)
  {
    setFilter((HotelsFilter) state.getParcelable(STATE_HOTEL_FILTER));
    updateFilterButtonVisibility(state.getBoolean(STATE_HOTEL_FILTER_VISIBILITY, false));
    mFilterView.onRestoreState(state, mFilter);
  }

  public static class DefaultFilterListener implements FilterListener
  {
    @Override
    public void onViewClick()
    {
    }

    @Override
    public void onFilterClick()
    {
    }

    @Override
    public void onFilterClear()
    {
    }

    @Override
    public void onFilterCancel()
    {
    }

    @Override
    public void onFilterDone()
    {
    }
  }
}
