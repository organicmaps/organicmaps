package com.mapswithme.maps.search;

import androidx.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.FeatureId;

/**
 * Native booking filter returns available hotels via this interface.
 */
@SuppressWarnings("unused")
public interface NativeBookingFilterListener
{
  /**
   * @param type Filter type which was applied.
   * @param hotels Array of hotels that meet the requirements for the filter.
   */
  void onFilterHotels(@BookingFilter.Type int type, @Nullable FeatureId[] hotels);
}
