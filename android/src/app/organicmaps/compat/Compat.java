package app.organicmaps.compat;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.widget.TimePicker;

import java.io.Serializable;

public interface Compat
{
  void setTime(TimePicker picker, int hour, int minute);
  int getHour(TimePicker picker);
  int getMinute(TimePicker picker);

  ResolveInfo resolveActivity(PackageManager packageManager, Intent intent, ResolveInfoFlags flags);

  <T extends Serializable> T getSerializableExtra(Intent intent, String name, Class<T> className);

  class ResolveInfoFlags
  {
    private long value;

    public ResolveInfoFlags(long value) {
      this.value = value;
    }

    public long getValue() {
      return value;
    }

    public static ResolveInfoFlags of(long value) {
      return new ResolveInfoFlags(value);
    }
  }

}
