package app.organicmaps.compat

import android.widget.TimePicker

interface Compat {
  fun setTime(picker: TimePicker, hour: Int, minute: Int)
  fun getHour(picker: TimePicker): Int
  fun getMinute(picker: TimePicker): Int
}