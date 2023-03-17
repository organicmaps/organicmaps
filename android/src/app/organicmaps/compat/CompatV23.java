package app.organicmaps.compat;

import android.annotation.TargetApi;
import android.util.Log;
import android.widget.TimePicker;

@TargetApi(23)
public class CompatV23 extends CompatV21 implements Compat {

  @Override
  public void setTime(TimePicker picker, int hour, int minute) {
    picker.setHour(hour);
    picker.setMinute(minute);
  }

  @Override
  public int getHour(TimePicker picker) {
    return picker.getHour();
  }

  @Override
  public int getMinute(TimePicker picker) {
    return picker.getMinute();
  }
}
