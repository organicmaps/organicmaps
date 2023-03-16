package app.organicmaps.compat

import android.widget.TimePicker

@Suppress("Deprecation")
open class CompatV21: Compat {

  override fun getHour(picker: TimePicker): Int {
    return picker.currentHour
  }

  override fun getMinute(picker: TimePicker): Int {
    return picker.currentMinute

  }

  override fun setTime(picker: TimePicker, hour: Int, minute: Int) {
    picker.currentHour = hour
    picker.currentMinute = minute
  }
}