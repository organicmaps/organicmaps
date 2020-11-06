package com.mapswithme.maps.search;

import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.ContextCompat;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

public class SearchFilterController implements SearchToolbarController.FilterParamsChangedListener
{
  private static final String STATE_HOTEL_FILTER = "state_hotel_filter";
  private static final String STATE_FILTER_PARAMS = "state_filter_params";
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
  private final SearchToolbarController mToolbarController;

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

  @Override
  public void onBookingParamsChanged()
  {
    mBookingFilterParams = mToolbarController.getFilterParams();
    if (mFilterListener != null)
      mFilterListener.onFilterParamsChanged();
  }

  public boolean isSatisfiedForSearch()
  {
    return mFilter != null || mBookingFilterParams != null;
  }

  interface FilterListener
  {
    void onShowOnMapClick();
    void onFilterClick();
    void onFilterClear();
    void onFilterParamsChanged();
  }

  SearchFilterController(@NonNull View frame, @Nullable FilterListener listener,
                         @NonNull SearchToolbarController toolbarController)
  {
    this(frame, listener, R.string.search_show_on_map, toolbarController);
  }

  public SearchFilterController(@NonNull View frame, @Nullable FilterListener listener,
                                @StringRes int populateButtonText,
                                @NonNull SearchToolbarController toolbarController)
  {
    mFrame = frame;
    mFilterListener = listener;
    mToolbarController = toolbarController;
    mToolbarController.addBookingParamsChangedListener(this);
    mShowOnMap = mFrame.findViewById(R.id.show_on_map);
    mShowOnMap.setText(populateButtonText);
    mFilterButton = mFrame.findViewById(R.id.filter_button);
    mFilterIcon = mFilterButton.findViewById(R.id.filter_icon);
    mFilterText = mFilterButton.findViewById(R.id.filter_text);
    mDivider = mFrame.findViewById(R.id.divider);

    initListeners();
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
    showPopulateButton(true);
  }

  void showPopulateButton(boolean show)
  {
    UiUtils.showIf(show, mShowOnMap);
  }

  void showDivider(boolean show)
  {
    UiUtils.showIf(show, mDivider);
  }

  public void updateFilterButtonsVisibility(boolean isHotel)
  {
    mHotelMode = isHotel;
    UiUtils.showIf(isHotel, mFilterButton);
    mToolbarController.showFilterControls(isHotel);
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

  public void setFilterParams(@Nullable BookingFilterParams params)
  {
    if (params == null)
      return;

    mToolbarController.setFilterParams(params);
    mBookingFilterParams = params;
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

  public void resetFilterAndParams()
  {
    setFilter(null);
    resetFilterParams();
    updateFilterButtonsVisibility(false);
  }

  private void resetFilterParams()
  {
    mToolbarController.resetFilterParams();
    mBookingFilterParams = null;
  }

  @Nullable
  public BookingFilterParams getBookingFilterParams()
  {
    return mBookingFilterParams;
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putParcelable(STATE_HOTEL_FILTER, mFilter);
    outState.putParcelable(STATE_FILTER_PARAMS, mBookingFilterParams);
    outState.putBoolean(STATE_HOTEL_FILTER_VISIBILITY,
                        mFilterButton.getVisibility() == View.VISIBLE);
  }

  public void onRestoreState(@NonNull Bundle state)
  {
    setFilter(state.getParcelable(STATE_HOTEL_FILTER));
    setFilterParams(state.getParcelable(STATE_FILTER_PARAMS));
    updateFilterButtonsVisibility(state.getBoolean(STATE_HOTEL_FILTER_VISIBILITY, false));
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

    @Override
    public void onFilterParamsChanged()
    {

    }
  }
}
