package com.mapswithme.maps.search;

/**
 * Native search will return results via this interface.
 */
@SuppressWarnings("unused")
public interface NativeSearchListener
{
  /**
   * @param count Count of results found.
   * @param timestamp Timestamp of search request.
   */
  void onResultsUpdate(int count, long timestamp);

  /**
   * @param timestamp Timestamp of search request.
   */
  void onResultsEnd(long timestamp);
}
