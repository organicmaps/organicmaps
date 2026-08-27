package app.organicmaps.intent.geo.handlers;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.intent.geo.navigation.Avoid;
import app.organicmaps.intent.geo.navigation.TravelMode;
import app.organicmaps.sdk.util.log.Logger;
import java.util.List;

public interface NavigationActionHandler
{
  String TAG = NavigationActionHandler.class.getSimpleName();

  /**
   * Starts turn-by-turn navigation to a destination by coordinates.
   *
   * <p>Build the route directly to {@code lat}/{@code lon}. Apply {@code avoid} and
   * {@code travelMode} when present.
   *
   * @param lat destination latitude.
   * @param lon destination longitude.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void navigate(double lat, double lon, @Nullable List<Avoid> avoid, @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: navigate to " + lat + ", " + lon + "avoid=" + avoid + ", travelMode=" + travelMode);
  }

  /**
   * Starts turn-by-turn navigation to a destination by query.
   *
   * <p>Resolve {@code query} to a destination, then start navigation to the resolved place. Apply
   * {@code avoid} and {@code travelMode} when present.
   *
   * @param query destination query.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void navigate(@NonNull String query, @Nullable List<Avoid> avoid, @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: navigate to " + query + "avoid=" + avoid + ", travelMode=" + travelMode);
  }

  /**
   * Starts turn-by-turn navigation to a destination by coordinates and query.
   *
   * <p>Resolve {@code query} to a destination with search biased to {@code lat}/{@code lon}, then
   * start navigation to the matched object. Apply {@code avoid} and {@code travelMode} when
   * present.
   *
   * @param lat destination latitude.
   * @param lon destination longitude.
   * @param query destination query.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void navigate(double lat, double lon, @NonNull String query, @Nullable List<Avoid> avoid,
                        @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: navigate to " + lat + ", " + lon + " (" + query + ")"
                      + "avoid=" + avoid + ", travelMode=" + travelMode);
  }

  /**
   * Adds a stop to the current route by coordinates.
   *
   * <p>Add the next stop at {@code lat}/{@code lon}.
   *
   * @param lat stop latitude.
   * @param lon stop longitude.
   */
  default void addStop(double lat, double lon)
  {
    Logger.d(TAG, "Not implemented: add stop at " + lat + ", " + lon);
  }

  /**
   * Adds a stop to the current route by query.
   *
   * <p>Resolve {@code query} to a place, then add that place as the next stop.
   *
   * @param query stop query.
   */
  default void addStop(@NonNull String query)
  {
    Logger.d(TAG, "Not implemented: add stop at " + query);
  }

  /**
   * Adds a stop to the current route by coordinates and query.
   *
   * <p>Resolve {@code query} to a stop with search biased to {@code lat}/{@code lon}, then add
   * the matched object as the next stop.
   *
   * @param lat stop latitude.
   * @param lon stop longitude.
   * @param query stop query.
   */
  default void addStop(double lat, double lon, @NonNull String query)
  {
    Logger.d(TAG, "Not implemented: add stop at " + lat + ", " + lon + " (" + query + ")");
  }

  /**
   * Shows directions to a destination by coordinates without starting navigation.
   *
   * <p>Build and present directions to {@code lat}/{@code lon}. Apply {@code avoid} and
   * {@code travelMode} when present.
   *
   * @param lat destination latitude.
   * @param lon destination longitude.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void showDirections(double lat, double lon, @Nullable List<Avoid> avoid, @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: show directions to " + lat + ", " + lon + "avoid=" + avoid
                      + ", travelMode=" + travelMode);
  }

  /**
   * Shows directions to a destination by query without starting navigation.
   *
   * <p>Resolve {@code query} to a destination, then show directions to the resolved place. Apply
   * {@code avoid} and {@code travelMode} when present.
   *
   * @param query destination query.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void showDirections(@NonNull String query, @Nullable List<Avoid> avoid, @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: show directions to " + query + "avoid=" + avoid + ", travelMode=" + travelMode);
  }

  /**
   * Shows directions to a destination by coordinates and query without starting navigation.
   *
   * <p>Resolve {@code query} to a destination with search biased to {@code lat}/{@code lon}, then
   * show directions to the matched object. Apply {@code avoid} and {@code travelMode} when
   * present.
   *
   * @param lat destination latitude.
   * @param lon destination longitude.
   * @param query destination query.
   * @param avoid route restrictions.
   * @param travelMode travel mode.
   */
  default void showDirections(double lat, double lon, @NonNull String query, @Nullable List<Avoid> avoid,
                              @Nullable TravelMode travelMode)
  {
    Logger.d(TAG, "Not implemented: show directions to " + lat + ", " + lon + " (" + query + ")"
                      + "avoid=" + avoid + ", travelMode=" + travelMode);
  }
}
