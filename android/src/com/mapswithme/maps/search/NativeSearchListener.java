package com.mapswithme.maps.search;

import androidx.annotation.NonNull;

/**
 * Native search will return results via this interface.
 */
@SuppressWarnings("unused")
public interface NativeSearchListener
{
  /**
   * @param results Search results.
   * @param timestamp Timestamp of search request.
   * @param isHotel Indicates that it's a hotel search result.
   */
  void onResultsUpdate(@NonNull SearchResult[] results, long timestamp, boolean isHotel);

  /**
   * @param timestamp Timestamp of search request.
   * @param isHotel Indicates that it's a hotel search result.
   */
  void onResultsEnd(long timestamp, boolean isHotel);
}
