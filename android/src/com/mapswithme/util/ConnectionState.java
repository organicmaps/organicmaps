package com.mapswithme.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.mapswithme.maps.MWMApplication;

public class ConnectionState
{
  private static boolean isNetworkConnected(int networkType)
  {
    final ConnectivityManager manager = (ConnectivityManager) MWMApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE);
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
}
