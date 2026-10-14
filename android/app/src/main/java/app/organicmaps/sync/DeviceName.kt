package app.organicmaps.sync

import android.content.Context
import android.os.Build
import android.provider.Settings

object DeviceName {
    // Settings.Global.DEVICE_NAME is API 25+, the setting itself exists on older devices too.
    private const val DEVICE_NAME = "device_name"

    /** The name the user gave the device, like "Pixel 6", for conflict copies of bookmark lists. */
    fun get(context: Context): String =
        Settings.Global.getString(context.contentResolver, DEVICE_NAME)?.takeIf { it.isNotBlank() } ?: Build.MODEL
}
