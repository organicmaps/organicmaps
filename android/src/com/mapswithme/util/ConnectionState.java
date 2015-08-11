package com.mapswithme.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.mapswithme.maps.MwmApplication;

public class ConnectionState
{
  // values should correspond to ones from enum class EConnectionType (in platform/platform.hpp)
  private static final byte CONNECTION_NONE = 0;
  private static final byte CONNECTION_WIFI = 1;
  private static final byte CONNECTION_WWAN = 2;

  private static boolean isNetworkConnected(int networkType)
  {
    final ConnectivityManager manager = (ConnectivityManager) MwmApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE);
    final NetworkInfo info = manager.getActiveNetworkInfo();
    return info != null && info.getType() == networkType && info.isConnected();
  }

  public static boolean is3gConnected()
  {
    return isNetworkConnected(ConnectivityManager.TYPE_MOBILE);
  }

  public static boolean isWifiConnected()
  {
    return isNetworkConnected(ConnectivityManager.TYPE_WIFI);
  }

  public static boolean isConnected()
  {
    return isNetworkConnected(ConnectivityManager.TYPE_WIFI) || isNetworkConnected(ConnectivityManager.TYPE_MOBILE);
  }

  public static byte getConnectionState()
  {
    if (isWifiConnected())
      return CONNECTION_WIFI;
    else if (is3gConnected())
      return CONNECTION_WWAN;

    return CONNECTION_NONE;
  }
}
