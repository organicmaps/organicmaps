package app.organicmaps.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;

import java.util.List;
import java.util.Locale;
import java.util.Map;

public class PowerManagerCompat
{
  private final String TAG = PowerManagerCompat.class.getSimpleName();
  public static final String HUAWEI = "HUAWEI";
  public static final String HUAWEI_POWER_MODE_INTENT = "huawei.intent.action.POWER_MODE_CHANGED_ACTION";
  public static final String HUAWEI_SETTING_NAME = "SmartModeStatus";
  public static final String XIAOMI = "XIAOMI";
  public static final String XIAOMI_POWER_MODE_INTENT = "miui.intent.action.POWER_SAVE_MODE_CHANGED";
  public static final String XIAOMI_SETTING_NAME = "POWER_SAVE_MODE_OPEN";
  private final Map<String, Integer> POWER_SAVE_MODE_VALUES = Map.of(HUAWEI, 4, XIAOMI, 1);
  private final List<String> POWER_SAVE_MODE_CHANGE_ACTIONS = List.of(HUAWEI_POWER_MODE_INTENT, XIAOMI_POWER_MODE_INTENT);
  private BroadcastReceiver mBroadcastReceiver;

  private BroadcastReceiver getBroadcastReceiver(PowerSaveModeChangeListener listener)
  {
    if (mBroadcastReceiver != null)
      return mBroadcastReceiver;

    mBroadcastReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        listener.onPowerSaveModeChanged();
      }
    };

    return mBroadcastReceiver;
  }

  // Subscribe to this broadcast reciever in order to get updates of state of batter saver mode of device
  public void monitorPowerSaveModeChanged(Context context, PowerSaveModeChangeListener listener)
  {
    if (POWER_SAVE_MODE_VALUES.containsKey(Build.MANUFACTURER.toUpperCase(Locale.getDefault())))
    {
      context.registerReceiver(getBroadcastReceiver(listener), new IntentFilter()
      {
        {
          for (String action : POWER_SAVE_MODE_CHANGE_ACTIONS)
            addAction(action);
        }
      });
    }
    else
    {
      context.registerReceiver(mBroadcastReceiver, new IntentFilter(PowerManager.ACTION_POWER_SAVE_MODE_CHANGED));
    }
  }

  public void unregister(Context context, PowerSaveModeChangeListener listener)
  {
    context.unregisterReceiver(getBroadcastReceiver(listener));
  }

  public boolean isPowerSaveMode(Context context)
  {
    return switch (Build.MANUFACTURER.toUpperCase(Locale.getDefault()))
    {
      case HUAWEI, XIAOMI -> isPowerSaveModeCompat(context);
      default ->
      {
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (pm != null) yield pm.isPowerSaveMode();
        else yield false;
      }
    };
  }

  private boolean isPowerSaveModeCompat(Context context)
  {
    return switch (Build.MANUFACTURER.toUpperCase(Locale.getDefault()))
    {
      case HUAWEI ->
          Settings.System.getInt(context.getContentResolver(), HUAWEI_SETTING_NAME, -1) == POWER_SAVE_MODE_VALUES.get(HUAWEI);
      case XIAOMI ->
          Settings.System.getInt(context.getContentResolver(), XIAOMI_SETTING_NAME, -1) == POWER_SAVE_MODE_VALUES.get(XIAOMI);
      default -> false;
    };
  }

  public interface PowerSaveModeChangeListener
  {
    //will be called when power save mode will change
    void onPowerSaveModeChanged();
  }
}
