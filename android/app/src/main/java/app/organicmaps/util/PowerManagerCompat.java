package app.organicmaps.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class PowerManagerCompat
{
  private final String TAG = "PowerManagementCompat";
  private final Map<String, Integer> POWER_SAVE_MODE_VALUES = new HashMap<String, Integer>()
  {
    {
      put("HUAWEI", 4);
      put("XIAOMI", 1);
    }
  };
  private final ArrayList<String> POWER_SAVE_MODE_SETTINGS_NAMES = new ArrayList<String>()
  {
    {
      add("SmartModeStatus"); // huawei setting name
      add("POWER_SAVE_MODE_OPEN"); // xiaomi setting name
    }
  };
  private final ArrayList<String> POWER_SAVE_MODE_CHANGE_ACTIONS = new ArrayList<String>()
  {
    {
      add("huawei.intent.action.POWER_MODE_CHANGED_ACTION");
      add("miui.intent.action.POWER_SAVE_MODE_CHANGED");
    }
  };

  public void monitorPowerSaveModeChanged(Context context, PowerSaveModeChangeListener listener)
  {
    if (POWER_SAVE_MODE_VALUES.containsKey(Build.MANUFACTURER.toUpperCase(Locale.getDefault())))
    {
      context.registerReceiver(new BroadcastReceiver()
      {
        @Override
        public void onReceive(Context context, Intent intent)
        {
          listener.onPowerSaveModeChanged();
        }
      }, new IntentFilter()
      {
        {
          for (String action : POWER_SAVE_MODE_CHANGE_ACTIONS)
            addAction(action);
        }
      });
    }
    else
    {
      context.registerReceiver(new BroadcastReceiver()
      {
        @Override
        public void onReceive(Context context, Intent intent)
        {
          listener.onPowerSaveModeChanged();
        }
      }, new IntentFilter(PowerManager.ACTION_POWER_SAVE_MODE_CHANGED));
    }
  }

  public void unregister(Context context, PowerSaveModeChangeListener listener)
  {
    context.unregisterReceiver(new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        listener.onPowerSaveModeChanged();
      }
    });
  }

  public boolean isPowerSaveMode(Context context)
  {
    if (POWER_SAVE_MODE_VALUES.containsKey(Build.MANUFACTURER.toUpperCase(Locale.getDefault())))
      return isPowerSaveModeCompat(context);
    PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
    return pm != null && pm.isPowerSaveMode();
  }

  private boolean isPowerSaveModeCompat(Context context)
  {
    for (String name : POWER_SAVE_MODE_SETTINGS_NAMES)
    {
      int mode = Settings.System.getInt(context.getContentResolver(), name, -1);
      if (mode != -1)
        return POWER_SAVE_MODE_VALUES.get(Build.MANUFACTURER.toUpperCase(Locale.getDefault())) == mode;
    }
    return false;
  }

  public interface PowerSaveModeChangeListener
  {
    //will be called when power save mode will change
    void onPowerSaveModeChanged();
  }
}
