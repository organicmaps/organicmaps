package app.organicmaps.sdk.widget.gui;

import android.content.Context;
import androidx.annotation.NonNull;

public class SpeedLimit
{
  public static void setRadius(@NonNull Context context, float radius)
  {
    final float density = context.getResources().getDisplayMetrics().density;
    nativeSetRadius(radius);
  }

  public static void setPosition(float x, float y)
  {
    nativeSetPosition(x, y);
  }

  public static void setEnabled(boolean enabled)
  {
    nativeSetEnabled(enabled);
  }

  private static native void nativeSetEnabled(boolean enabled);
  private static native void nativeSetRadius(float radius);
  private static native void nativeSetPosition(float x, float y);
}
