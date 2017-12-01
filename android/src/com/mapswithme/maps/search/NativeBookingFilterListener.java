package com.mapswithme.maps.search;

import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.FeatureId;

/**
 * Native booking filter returns available hotels via this interface.
 */
@SuppressWarnings("unused")
public interface NativeBookingFilterListener
{
  /**
   * @param availableHotels Array of available hotels feature ids.
   */
  void onFilterAvailableHotels(@Nullable FeatureId[] availableHotels);
}
