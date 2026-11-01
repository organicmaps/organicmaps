package app.organicmaps.sdk.search;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Main-thread callbacks from {@link SearchEngine#searchMaps} for downloadable region names.
 */
public interface MapSearchListener
{
  /**
   * A region-name match.
   * @param countryId native region identifier accepted by the downloader
   * @param matchedString matching localized name
   */
  @Keep
  @SuppressWarnings("unused")
  record Result(String countryId, String matchedString)
  {}

  /**
   * Replaces the current matching-region snapshot.
   * @param results current region matches
   * @param timestamp identifier supplied to the request
   * @param isLast whether this is the final snapshot
   */
  @Keep
  @SuppressWarnings("unused")
  void onMapSearchResults(@NonNull Result[] results, long timestamp, boolean isLast);
}
