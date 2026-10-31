package app.organicmaps.sdk.search;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Main-thread ordinary-search callbacks. Timestamps identify requests; ignore callbacks for superseded requests.
 */
public interface SearchListener
{
  /**
   * Replaces the current result snapshot.
   * @param results complete current snapshot, including suggestions; do not append it to the previous snapshot
   * @param timestamp identifier supplied when the search was started
   */
  @Keep
  @SuppressWarnings("unused")
  default void onResultsUpdate(@NonNull SearchResult[] results, long timestamp)
  {}

  /**
   * Signals completion of a search request.
   * @param timestamp identifier supplied when the search was started
   */
  @Keep
  @SuppressWarnings("unused")
  default void onResultsEnd(long timestamp)
  {}
}
