package app.organicmaps.sdk.car.renderer;

import androidx.annotation.Nullable;
import androidx.core.util.Consumer;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController;

public interface Renderer
{
  boolean isRenderingActive();

  void enable();
  void disable();

  void onZoomIn();
  void onZoomOut();

  @Nullable
  CombinedSpeedViewController getSpeedLimitController();

  /** Called immediately if available, and again whenever a presentation is created. */
  void setSpeedLimitControllerListener(@Nullable Consumer<CombinedSpeedViewController> listener);
}
