package app.organicmaps.sdk.car.renderer;

public interface Renderer
{
  boolean isRenderingActive();

  void enable();
  void disable();

  void onZoomIn();
  void onZoomOut();

  /**
   * Updates speed limit view.
   *
   * @param speedLimit         The speed limit value in formatted units. A value <= 0 will hide the view.
   * @param speedLimitExceeded True if the current speed exceeds the speed limit, false otherwise.
   */
  void setSpeedLimit(int speedLimit, boolean speedLimitExceeded);
  void hideSpeedLimit();
}
