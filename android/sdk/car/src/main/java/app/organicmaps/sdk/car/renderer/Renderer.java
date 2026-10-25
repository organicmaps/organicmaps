package app.organicmaps.sdk.car.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface Renderer
{
  boolean isRenderingActive();

  void enable();
  void disable();

  void onZoomIn();
  void onZoomOut();

  /** Retains the current speed and localized units, including while no presentation exists. */
  void setCurrentSpeed(int speed, @NonNull String units);

  /** Retains the limit in the current speed units; null hides the limit. */
  void setSpeedLimit(@Nullable Integer speedLimit);

  /** Retains an alert override, e.g. when a speed-camera limit is exceeded. */
  void setAlertForced(boolean alertForced);
}
