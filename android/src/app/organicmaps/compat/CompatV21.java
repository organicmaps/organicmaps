package app.organicmaps.compat;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.widget.TimePicker;

@SuppressWarnings("deprecation")
public class CompatV21 implements Compat {

  @Override
  public int getHour(TimePicker picker) {
    return picker.getCurrentHour();
  }

  @Override
  public int getMinute(TimePicker picker) {
    return picker.getCurrentMinute();
  }

  @Override
  public void setTime(TimePicker picker, int hour, int minute) {
    picker.setCurrentHour(hour);
    picker.setCurrentMinute(minute);
  }

  @Override
  public ResolveInfo resolveActivity(PackageManager packageManager, Intent intent, Compat.ResolveInfoFlags flags) {
    return packageManager.resolveActivity(intent, (int) flags.getValue());
  }
}