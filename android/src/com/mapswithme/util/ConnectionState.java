package com.mapswithme.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.telephony.TelephonyManager;

import com.mapswithme.maps.MwmApplication;

import static com.mapswithme.util.ConnectionState.Type.NONE;

public class ConnectionState
{
  // values should correspond to ones from enum class EConnectionType (in platform/platform.hpp)
  private static final byte CONNECTION_NONE = 0;
  private static final byte CONNECTION_WIFI = 1;
  private static final byte CONNECTION_WWAN = 2;

  public enum Type
  {
    NONE(CONNECTION_NONE, -1),
    WIFI(CONNECTION_WIFI, ConnectivityManager.TYPE_WIFI),
    WWAN(CONNECTION_WWAN, ConnectivityManager.TYPE_MOBILE);

    private final byte mValue;
    private final int mNetwork;

    Type(byte value, int network)
    {
      mValue = value;
      mNetwork = network;
    }

    public byte getValue()
    {
      return mValue;
    }

    public int getNetwork()
    {
      return mNetwork;
    }
  }

  private static boolean isNetworkConnected(int networkType)
  {
    final NetworkInfo info = getActiveNetwork();
    return info != null && info.getType() == networkType && info.isConnected();
  }

  public static
  @Nullable
  NetworkInfo getActiveNetwork()
  {
    return ((ConnectivityManager) MwmApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE)).getActiveNetworkInfo();
  }

  public static boolean isMobileConnected()
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

  public static boolean isConnectionFast(NetworkInfo info)
  {
    if (info == null || !info.isConnected())
      return false;

    final int type = info.getType();
    final int subtype = info.getSubtype();

    if (type == ConnectivityManager.TYPE_WIFI)
      return true;

    if (type == ConnectivityManager.TYPE_MOBILE)
    {
      switch (subtype)
      {
      case TelephonyManager.NETWORK_TYPE_IDEN: // ~25 kbps
      case TelephonyManager.NETWORK_TYPE_CDMA: // ~ 14-64 kbps
      case TelephonyManager.NETWORK_TYPE_1xRTT: // ~ 50-100 kbps
      case TelephonyManager.NETWORK_TYPE_EDGE: // ~ 50-100 kbps
      case TelephonyManager.NETWORK_TYPE_GPRS: // ~ 100 kbps
      case TelephonyManager.NETWORK_TYPE_UNKNOWN:
        return false;
      case TelephonyManager.NETWORK_TYPE_EVDO_0: // ~ 400-1000 kbps
      case TelephonyManager.NETWORK_TYPE_EVDO_A: // ~ 600-1400 kbps
      case TelephonyManager.NETWORK_TYPE_HSDPA: // ~ 2-14 Mbps
      case TelephonyManager.NETWORK_TYPE_HSPA: // ~ 700-1700 kbps
      case TelephonyManager.NETWORK_TYPE_HSUPA: // ~ 1-23 Mbps
      case TelephonyManager.NETWORK_TYPE_UMTS: // ~ 400-7000 kbps
      case TelephonyManager.NETWORK_TYPE_EHRPD: // ~ 1-2 Mbps
      case TelephonyManager.NETWORK_TYPE_EVDO_B: // ~ 5 Mbps
      case TelephonyManager.NETWORK_TYPE_HSPAP: // ~ 10-20 Mbps
      case TelephonyManager.NETWORK_TYPE_LTE: // ~ 10+ Mbps
      default:
        return true;
      }
    }

    return false;
  }

  public static boolean isInRoaming()
  {
    NetworkInfo info = getActiveNetwork();
    return info != null && info.isRoaming();
  }

  public static byte getConnectionState()
  {
    return requestCurrentType().getValue();
  }

  @NonNull
  public static Type requestCurrentType()
  {
    for (ConnectionState.Type each : ConnectionState.Type.values())
    {
      if (isNetworkConnected(each.getNetwork()))
        return each;
    }
    return NONE;
  }
}
