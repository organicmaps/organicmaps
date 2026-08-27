package app.organicmaps.intent;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import androidx.annotation.NonNull;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public final class IntentUtils
{
  private final static double[] INVALID_COORDINATES = new double[] {Double.NaN, Double.NaN};

  @NonNull
  public static double[] getCoordinates(@NonNull final Intent intent)
  {
    final Uri data = intent.getData();
    if (data == null)
      return INVALID_COORDINATES;

    final String ssp = data.getSchemeSpecificPart();
    if (ssp == null || ssp.isEmpty())
      return INVALID_COORDINATES;

    int endOfCoordinates = ssp.indexOf('?');
    if (endOfCoordinates < 0)
      endOfCoordinates = ssp.length();

    final String latLon = ssp.substring(0, endOfCoordinates);
    final String[] parts = latLon.split(",");
    if (parts.length != 2)
      return INVALID_COORDINATES;

    final double lat = Double.parseDouble(parts[0]);
    final double lon = Double.parseDouble(parts[1]);
    return new double[] {lat, lon};
  }

  @NonNull
  public static Map<String, String> getQueryParameters(@NonNull final Intent intent)
  {
    final Uri data = intent.getData();
    if (data == null)
      return Collections.emptyMap();

    if (data.isHierarchical())
      return getQueryParametersForHierarchicalUri(data);

    return getQueryParametersForNonHierarchicalUri(data);
  }

  @NonNull
  private static Map<String, String> getQueryParametersForHierarchicalUri(@NonNull final Uri data)
  {
    final Set<String> keys = data.getQueryParameterNames();
    if (keys.isEmpty())
      return Collections.emptyMap();

    final Map<String, String> parameters = new HashMap<>(keys.size());
    for (final String key : keys)
      parameters.put(key, data.getQueryParameter(key));
    return parameters;
  }

  @NonNull
  private static Map<String, String> getQueryParametersForNonHierarchicalUri(@NonNull final Uri data)
  {
    final String ssp = data.getSchemeSpecificPart();
    if (ssp == null)
      return Collections.emptyMap();

    final int q = ssp.indexOf('?');
    if (q < 0 || q + 1 >= ssp.length())
      return Collections.emptyMap();

    final String query = ssp.substring(q + 1);
    final Map<String, String> parameters = new HashMap<>();
    for (final String pair : query.split("&"))
    {
      final int eq = pair.indexOf('=');
      if (eq <= 0)
        continue;
      final String key = pair.substring(0, eq);
      final String value = decodeUrl(pair.substring(eq + 1));
      parameters.put(key, value);
    }
    return parameters;
  }

  @NonNull
  private static String decodeUrl(@NonNull final String value)
  {
    if (Build.VERSION.SDK_INT >= 33)
      return URLDecoder.decode(value, StandardCharsets.UTF_8);

    try
    {
      return URLDecoder.decode(value, "UTF-8");
    }
    catch (UnsupportedEncodingException e)
    {
      return value;
    }
  }

  private IntentUtils() {}
}
