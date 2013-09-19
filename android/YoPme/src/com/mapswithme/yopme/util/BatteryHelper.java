package com.mapswithme.yopme.util;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;

public class BatteryHelper
{
  public final static float BATTERY_LEVEL_CRITICAL = 5f;
  public final static float BATTERY_LEVEL_LOW = 15f;

  public enum BatteryLevel
  {
    CRITICAL,
    LOW,
    OK,
  }


  public static float getBatteryLevel(Context context)
  {
    final IntentFilter batteryIntentFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
    final Intent batteryIntent = context.registerReceiver(null, batteryIntentFilter);

    if (batteryIntent == null)
      return 0; // should not happen, by the way

    final int level = batteryIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
    final int scale = batteryIntent.getIntExtra(BatteryManager.EXTRA_SCALE, 1);

    return 100f*((float)level/(float)scale);
  }

  public static BatteryLevel getBatteryLevelRange(Context context)
  {
    final float level = getBatteryLevel(context);
    if (level <= BATTERY_LEVEL_CRITICAL)
      return BatteryLevel.CRITICAL;
    if (level <= BATTERY_LEVEL_LOW)
      return BatteryLevel.LOW;

    return BatteryLevel.OK;
  }


  private BatteryHelper() {};
}
