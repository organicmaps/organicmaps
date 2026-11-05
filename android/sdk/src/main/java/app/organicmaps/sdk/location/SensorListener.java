package app.organicmaps.sdk.location;

public interface SensorListener
{
  /**
   * Receives the display heading in [0, 2π) radians clockwise from true north.
   * Before the first accepted location, the heading uses magnetic north as an approximate fallback.
   */
  void onCompassUpdated(double north);

  default void onCompassCalibrationRecommended()
  {
    // No op.
  }

  default void onCompassCalibrationRequired()
  {
    // No op.
  }
}
