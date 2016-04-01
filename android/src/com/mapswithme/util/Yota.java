package com.mapswithme.util;

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
import android.text.TextUtils;

import com.mapswithme.maps.location.LocationHelper;

public class Yota
{
  public static boolean isFirstYota()
  {
    return Build.DEVICE.equals(Constants.DEVICE_YOTAPHONE);
  }

  private final static String YOPME_AUTHORITY = "com.mapswithme.yopme";
  public final static String ACTION_PREFERENCE = YOPME_AUTHORITY + ".preference";
  public final static String ACTION_SHOW_RECT = YOPME_AUTHORITY + ".show_rect";
  public final static String ACTION_LOCATION = YOPME_AUTHORITY + ".location";

  public final static String EXTRA_LAT = YOPME_AUTHORITY + ".lat";
  public final static String EXTRA_LON = YOPME_AUTHORITY + ".lon";
  public final static String EXTRA_ZOOM = YOPME_AUTHORITY + ".zoom";
  public final static String EXTRA_NAME = YOPME_AUTHORITY + ".name";
  public final static String EXTRA_MODE = YOPME_AUTHORITY + ".mode";
  public final static String EXTRA_IS_POI = YOPME_AUTHORITY + ".is_poi";

  public final static String EXTRA_HAS_LOCATION = YOPME_AUTHORITY + ".haslocation";
  public final static String EXTRA_MY_LAT = YOPME_AUTHORITY + ".mylat";
  public final static String EXTRA_MY_LON = YOPME_AUTHORITY + ".mylon";


  public static void showLocation(Context context, double zoom)
  {
    final Intent locationYotaIntent = populateIntent(ACTION_LOCATION, 0, 0, zoom, null, true, false);
    context.startService(locationYotaIntent);
  }

  public static void showMap(Context context, double vpLat, double vpLon, double zoom, String name, boolean addLastKnown)
  {
    final Intent poiYotaIntent = populateIntent(ACTION_SHOW_RECT, vpLat, vpLon,
        zoom, name, addLastKnown,
        !TextUtils.isEmpty(name));
    context.startService(poiYotaIntent);
  }

  private static Intent populateIntent(String action, double lat, double lon,
                                       double zoom, String name,
                                       boolean addLastKnow, boolean isPoi)
  {
    final Intent i = new Intent(action)
        .putExtra(EXTRA_LAT, lat)
        .putExtra(EXTRA_LON, lon)
        .putExtra(EXTRA_ZOOM, zoom)
        .putExtra(EXTRA_NAME, name)
        .putExtra(EXTRA_IS_POI, isPoi);

    if (addLastKnow)
    {
      final Location lastLocation = LocationHelper.INSTANCE.getSavedLocation();
      if (lastLocation != null)
      {
        i.putExtra(EXTRA_HAS_LOCATION, true)
            .putExtra(EXTRA_MY_LAT, lastLocation.getLatitude())
            .putExtra(EXTRA_MY_LON, lastLocation.getLongitude());
      }
      else
        i.putExtra(EXTRA_HAS_LOCATION, false);
    }

    return i;
  }

  private Yota() {}
}
