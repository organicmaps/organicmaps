package com.mapswithme.maps.search;

import androidx.annotation.Nullable;

/**
 * Native search will return results via this interface.
 */
@SuppressWarnings("unused")
public interface NativeBookmarkSearchListener
{
  /**
   * @param bookmarkIds Founded bookmark ids.
   * @param timestamp Timestamp of search request.
   */
  void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp);

  /**
   * @param bookmarkIds Founded bookmark ids.
   * @param timestamp Timestamp of search request.
   */
  void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp);
}
