package app.organicmaps.routing;

import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.BuildConfig;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RoutingController;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Locale;

final class RoutingErrorDiagnostics
{
  private RoutingErrorDiagnostics() {}

  @NonNull
  static String capture(int resultCode, @Nullable String[] missingMaps)
  {
    RouteMarkData[] points = Framework.nativeGetRoutePoints();
    String[] regions = new String[points.length];
    for (int i = 0; i < points.length; ++i)
    {
      String country = MapManager.nativeFindCountry(points[i].mLat, points[i].mLon);
      if (country == null || country.isEmpty())
        regions[i] = "unknown";
      else
      {
        CountryItem item = CountryItem.fill(country);
        regions[i] = country + " [downloaded=" + item.present + ", status=" + item.status + "]";
      }
    }
    return format(BuildConfig.VERSION_NAME, RoutingController.get().getLastRouterType(), resultCode, points, regions)
  + "\nAndroid: " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + "), " + Build.MANUFACTURER + " "
  + Build.MODEL + "\nMap catalog: " + new SimpleDateFormat("yyyy-MM-dd", Locale.ROOT).format(Framework.getDataVersion())
  + "\nMissing maps: " + (missingMaps == null ? "[]" : Arrays.toString(missingMaps));
  }

  @NonNull
  static String format(@NonNull String version, @NonNull Router router, int resultCode, @NonNull RouteMarkData[] points,
                       @NonNull String[] regions)
  {
    StringBuilder text = new StringBuilder("Organic Maps routing error\nApp: ")
                             .append(version)
                             .append("\nMode: ")
                             .append(router.name())
                             .append("\nResult code: ")
                             .append(resultCode);
    for (int i = 0; i < points.length; ++i)
    {
      RouteMarkData point = points[i];
      String label = switch (point.mPointType)
      {
        case Start -> "Start";
        case Finish -> "Finish";
        case Intermediate -> "Stop " + (point.mIntermediateIndex + 1);
      };
      text.append(String.format(Locale.ROOT, "\n%s: %.8f, %.8f", label, point.mLat, point.mLon));
      if (point.mIsMyPosition)
        text.append(" (My position)");
      text.append("\n  Map: ").append(regions[i]);
    }
    return text.toString();
  }
}
