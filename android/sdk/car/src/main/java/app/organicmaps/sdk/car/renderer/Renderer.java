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
   * @param speedLimit   Speed limit value in formatted units. A value <= 0 will hide the view.
   * @param currentSpeed Current speed value in formatted units.
   */
  void setSpeedLimit(int speedLimit, int currentSpeed);
  void hideSpeedLimit();
}
