package app.organicmaps.compat

import android.annotation.TargetApi
import android.widget.TimePicker

@TargetApi(23)
open class CompatV23: CompatV21(), Compat {

  override fun setTime(picker: TimePicker, hour: Int, minute: Int) {
    picker.hour = hour
    picker.minute = minute
  }

  override fun getHour(picker: TimePicker): Int {
    return picker.hour
  }

  override fun getMinute(picker: TimePicker): Int {
    return picker.minute
  }
}