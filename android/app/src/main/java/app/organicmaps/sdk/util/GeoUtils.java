package app.organicmaps.sdk.util;

import android.graphics.PointF;
import androidx.annotation.NonNull;

public class GeoUtils
{
  @NonNull
  static public PointF toLatLon(double merX, double merY)
  {
    return nativeToLatLon(merX, merY);
  }

  @NonNull
  private static native PointF nativeToLatLon(double merX, double merY);
}
