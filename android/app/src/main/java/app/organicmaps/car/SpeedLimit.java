package app.organicmaps.car;

import android.content.Context;
import android.graphics.Rect;
import androidx.annotation.NonNull;

public class SpeedLimit
{
  private static final int WIDGET_RADIUS_DP = 60;

  private float mWidgetRadiusPx = 0;

  public void setEnabled(Context context, boolean enabled)
  {
    if (enabled)
    {
      mWidgetRadiusPx = context.getResources().getDisplayMetrics().density * WIDGET_RADIUS_DP * 0.5f;
      app.organicmaps.sdk.widget.gui.SpeedLimit.setRadius(context, mWidgetRadiusPx);
    }
    app.organicmaps.sdk.widget.gui.SpeedLimit.setEnabled(enabled);
  }

  public void updatePosition(@NonNull Rect visibleArea)
  {
    final float x = visibleArea.right - mWidgetRadiusPx;
    final float y = visibleArea.top + mWidgetRadiusPx;
    app.organicmaps.sdk.widget.gui.SpeedLimit.setPosition(x, y);
  }
}
