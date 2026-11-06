package app.organicmaps.sdk.car.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;

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

  /** Retains the region's sign type, including while no presentation exists. */
  void setSpeedLimitSignType(@Framework.SpeedLimitSignType int type);

  /** Retains an alert override, e.g. when a speed-camera limit is exceeded. */
  void setAlertForced(boolean alertForced);
}
