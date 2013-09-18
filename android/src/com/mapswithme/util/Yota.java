package com.mapswithme.util;

import com.mapswithme.maps.MWMApplication;

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Build;

public class Yota
{
  public static boolean isYota()
  {
    return Build.DEVICE.contains("yotaphone");
  }

  private final static String YOPME_AUTHORITY   = "com.mapswithme.yopme";
  public  final static String ACTION_PREFERENCE = YOPME_AUTHORITY + ".preference";
  public  final static String ACTION_SHOW_RECT  = YOPME_AUTHORITY + ".show_rect";
  public  final static String ACTION_LOCATION   = YOPME_AUTHORITY + ".location";

  public  final static String EXTRA_LAT    = YOPME_AUTHORITY + ".lat";
  public  final static String EXTRA_LON    = YOPME_AUTHORITY + ".lon";
  public  final static String EXTRA_ZOOM   = YOPME_AUTHORITY + ".zoom";
  public  final static String EXTRA_NAME   = YOPME_AUTHORITY + ".name";
  public  final static String EXTRA_MODE   = YOPME_AUTHORITY + ".mode";

  public  final static String EXTRA_HAS_LOCATION   = YOPME_AUTHORITY + ".haslocation";
  public  final static String EXTRA_MY_LAT   = YOPME_AUTHORITY + ".mylat";
  public  final static String EXTRA_MY_LON   = YOPME_AUTHORITY + ".mylon";



  public static void showLocation(Context context, double zoom)
  {
    final Intent locationYotaIntent = populateIntent(ACTION_LOCATION, 0, 0, zoom, null, true);
    context.startService(locationYotaIntent);
  }

  public static void showPoi(Context context, double lat, double lon, double zoom, String name, boolean addLastKnown)
  {
    final Intent poiYotaIntent = populateIntent(ACTION_SHOW_RECT, lat, lon, zoom, name, addLastKnown);
    context.startService(poiYotaIntent);
  }

  private static Intent populateIntent(String action, double lat, double lon,
      double zoom, String name, boolean addLastKnow)
  {
    final Intent i = new Intent(action)
    .putExtra(EXTRA_LAT, lat)
    .putExtra(EXTRA_LON, lon)
    .putExtra(EXTRA_ZOOM, zoom)
    .putExtra(EXTRA_NAME, name);

    if (addLastKnow)
    {
      final Location lastLocation = MWMApplication.get().getLocationService().getLastKnown();
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

  private Yota() {};
}
