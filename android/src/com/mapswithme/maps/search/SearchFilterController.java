package com.mapswithme.maps.search;

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

  @Nullable
  private HotelsFilter mFilter;
  @Nullable
  private BookingFilterParams mBookingFilterParams;
  private boolean mHotelMode;

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
    void onShowOnMapClick();
    void onFilterClick();
    void onFilterClear();
  }

  SearchFilterController(@NonNull View frame, @Nullable FilterListener listener)
  {
    this(frame, listener, R.string.search_show_on_map);
  }

  public SearchFilterController(@NonNull View frame,
                                @Nullable FilterListener listener, @StringRes int populateButtonText)
  {
    mFrame = frame;
    mFilterListener = listener;
    mShowOnMap = mFrame.findViewById(R.id.show_on_map);
    mShowOnMap.setText(populateButtonText);
    mFilterButton = mFrame.findViewById(R.id.filter_button);
    mFilterIcon = mFilterButton.findViewById(R.id.filter_icon);
    mFilterText = mFilterButton.findViewById(R.id.filter_text);
    mDivider = mFrame.findViewById(R.id.divider);

    initListeners();
  }

  public void show(boolean show, boolean showPopulateButton)
  {
    UiUtils.showIf(show && (showPopulateButton || mHotelMode), mFrame);
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
    mHotelMode = isHotel;
    UiUtils.showIf(isHotel, mFilterButton);
  }

  private void initListeners()
  {
    mShowOnMap.setOnClickListener(v ->
                                  {
                                    if (mFilterListener != null)
                                      mFilterListener.onShowOnMapClick();
                                  });
    mFilterButton.setOnClickListener(v ->
                                     {
                                       if (mFilterListener != null)
                                         mFilterListener.onFilterClick();
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

  void resetFilter()
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

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putParcelable(STATE_HOTEL_FILTER, mFilter);
    outState.putBoolean(STATE_HOTEL_FILTER_VISIBILITY,
                        mFilterButton.getVisibility() == View.VISIBLE);
  }

  public void onRestoreState(@NonNull Bundle state)
  {
    setFilter(state.getParcelable(STATE_HOTEL_FILTER));
    updateFilterButtonVisibility(state.getBoolean(STATE_HOTEL_FILTER_VISIBILITY, false));
  }

  public static class DefaultFilterListener implements FilterListener
  {
    @Override
    public void onShowOnMapClick()
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
  }
}
