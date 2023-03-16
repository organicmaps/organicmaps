package app.organicmaps.compat

import android.os.Build

class CompatHelper private constructor() {

  private val compatValue: Compat = when {
    sdkVersion >= Build.VERSION_CODES.M -> CompatV23()
    else -> CompatV21()
  }

  companion object {

    private val instance by lazy { CompatHelper() }

    /** Get the current Android API level.  */
    val sdkVersion: Int
      get() = Build.VERSION.SDK_INT

    val compat get() = instance.compatValue
  }

}