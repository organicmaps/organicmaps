package com.mapswithme.util;

import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class BatteryState
{
  public static final byte CHARGING_STATUS_UNKNOWN = 0;
  public static final byte CHARGING_STATUS_PLUGGED = 1;
  public static final byte CHARGING_STATUS_UNPLUGGED = 2;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CHARGING_STATUS_UNKNOWN, CHARGING_STATUS_PLUGGED, CHARGING_STATUS_UNPLUGGED })
  public @interface ChargingStatus
  {
  }

  private BatteryState() {}

  @ChargingStatus
  public static int getChargingStatus()
  {
    IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
    Intent batteryStatus = MwmApplication.get().registerReceiver(null, filter);
    if (batteryStatus == null)
      return CHARGING_STATUS_UNKNOWN;

    return getChargingStatus(batteryStatus);
  }

  @ChargingStatus
  public static int getChargingStatus(@NonNull Intent batteryStatus)
  {
    int chargePlug = batteryStatus.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
    if (chargePlug > 0)
      return CHARGING_STATUS_PLUGGED;
    else if (chargePlug < 0)
      return CHARGING_STATUS_UNKNOWN;

    return CHARGING_STATUS_UNPLUGGED;
  }
}
