package app.organicmaps.search;

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
   */
  void onResultsUpdate(@NonNull SearchResult[] results, long timestamp);

  /**
   * @param timestamp Timestamp of search request.
   */
  default void onResultsEnd(long timestamp) {}
}
