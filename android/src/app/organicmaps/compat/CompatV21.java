package app.organicmaps.compat;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.widget.TimePicker;

import java.io.Serializable;

@SuppressWarnings("deprecation")
public class CompatV21 implements Compat
{

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

  @Override
  public <T extends Serializable> T getSerializableExtra(Intent intent, String name, Class<T> className) {
    try {
      return className.cast(intent.getSerializableExtra(name));
    } catch (Exception e) {
      return null;
    }
  }

}