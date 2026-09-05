package app.organicmaps.intent.geo.handlers;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.log.Logger;

public interface SearchActionHandler
{
  String TAG = SearchActionHandler.class.getSimpleName();

  /**
   * Runs a map search by query.
   *
   * @param query search query.
   */
  default void search(@NonNull String query)
  {
    Logger.d(TAG, "Not implemented: search for " + query);
  }

  /**
   * Runs a map search by query with location bias.
   *
   * <p>Use {@code lat}/{@code lon} as the search bias and show results for {@code query}.
   *
   * @param lat bias latitude.
   * @param lon bias longitude.
   * @param query search query.
   */
  default void search(double lat, double lon, @NonNull String query)
  {
    Logger.d(TAG, "Not implemented: search for " + query + " at " + lat + ", " + lon);
  }

  /** Closes search results if they are currently shown. */
  default void clearSearchResults()
  {
    Logger.d(TAG, "Not implemented: clear search results");
  }

  /**
   * Selects a result from the current search results list.
   *
   * @param index zero-based result index.
   */
  default void selectSearchResult(int index)
  {
    Logger.d(TAG, "Not implemented: select search result index=" + index);
  }

  /**
   * Enables or disables electric vehicle connector filtering for search results.
   *
   * @param enable {@code true} to enable the filter, {@code false} to remove it.
   */
  default void enableElectricVehicleConnectorFilter(boolean enable)
  {
    Logger.d(TAG, "Not implemented: enable electric vehicle connector filter enable=" + enable);
  }

  /**
   * Enables or disables electric vehicle payment filtering for search results.
   *
   * @param enable {@code true} to enable the filter, {@code false} to remove it.
   */
  default void enableElectricVehiclePaymentFilter(boolean enable)
  {
    Logger.d(TAG, "Not implemented: enable electric vehicle payment filter enable=" + enable);
  }

  /**
   * Enables or disables electric vehicle fast charging filtering for search results.
   *
   * @param enable {@code true} to enable the filter, {@code false} to remove it.
   */
  default void enableElectricVehicleFastChargingFilter(boolean enable)
  {
    Logger.d(TAG, "Not implemented: enable electric vehicle fast charging filter enable=" + enable);
  }
}
