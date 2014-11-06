package com.mapswithme.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.mapswithme.maps.MWMApplication;

public class ConnectionState
{
  public static final int NOT_CONNECTED = 0;
  public static final int CONNECTED_BY_3G = 1;
  public static final int CONNECTED_BY_WIFI = 2;
  public static final int CONNECTED_BY_WIFI_AND_3G = CONNECTED_BY_3G & CONNECTED_BY_WIFI;
  private static final String TYPE_WIFI = "WIFI";
  private static final String TYPE_MOBILE = "MOBILE";

  private static int getState()
  {
    boolean isWifiConnected = false;
    boolean isMobileConnected = false;

    ConnectivityManager cm = (ConnectivityManager) MWMApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo[] netInfo = cm.getAllNetworkInfo();
    for (NetworkInfo ni : netInfo)
    {
      if (ni.getTypeName().equalsIgnoreCase(TYPE_WIFI) && ni.isConnected())
          isWifiConnected = true;
      if (ni.getTypeName().equalsIgnoreCase(TYPE_MOBILE) && ni.isConnected())
          isMobileConnected = true;
    }
    if (isWifiConnected && isMobileConnected)
      return CONNECTED_BY_WIFI_AND_3G;
    else if (isMobileConnected)
      return CONNECTED_BY_3G;
    else if (isWifiConnected)
      return CONNECTED_BY_WIFI;

    return NOT_CONNECTED;
  }

  public static boolean is3GConnected()
  {
    return (getState() & CONNECTED_BY_3G) == CONNECTED_BY_3G;
  }

  public static boolean isWifiConnected()
  {
    return (getState() & CONNECTED_BY_WIFI) == CONNECTED_BY_WIFI;
  }

  public static boolean isConnected()
  {
    return getState() != NOT_CONNECTED;
  }
}
