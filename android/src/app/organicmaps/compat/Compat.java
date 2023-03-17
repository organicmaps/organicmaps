package app.organicmaps.compat;

import android.widget.TimePicker;

public interface Compat
{
  void setTime(TimePicker picker, int hour, int minute);
  int getHour(TimePicker picker);
  int getMinute(TimePicker picker);

}
