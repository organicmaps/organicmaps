package app.organicmaps.syria.tracking

import android.app.Activity
import android.app.ActivityManager
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.PowerManager
import android.provider.Settings
import org.json.JSONObject

/**
 * The app's battery setting, which decides whether a recording keeps going with the screen off:
 * Unrestricted (no battery optimization), Optimized (the default: Android may delay or stop it) or
 * Restricted (stopped in the background). Phone makers' own savers (Xiaomi Autostart, Huawei App launch,
 * Samsung sleeping apps) cannot be read: the maker and model are sent with the mode to explain them.
 */
enum class BackgroundMode(val key: String) {
    UNRESTRICTED("unrestricted"),
    OPTIMIZED("optimized"),
    RESTRICTED("restricted"),
    ;

    companion object {
        @JvmStatic
        fun current(context: Context): BackgroundMode {
            val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && activityManager?.isBackgroundRestricted == true) {
                return RESTRICTED
            }
            // No battery optimization before Android 6.
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return UNRESTRICTED
            val power = context.getSystemService(Context.POWER_SERVICE) as? PowerManager ?: return OPTIMIZED
            return if (power.isIgnoringBatteryOptimizations(context.packageName)) UNRESTRICTED else OPTIMIZED
        }

        /** Event payload: the mode with the phone it applies to. */
        fun payload(context: Context, mode: BackgroundMode = current(context)): JSONObject {
            val power = context.getSystemService(Context.POWER_SERVICE) as? PowerManager
            return JSONObject()
                .put("batteryMode", mode.key)
                .put("powerSaver", power?.isPowerSaveMode == true)
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("androidSdk", Build.VERSION.SDK_INT)
        }

        /** App info, where Battery > Unrestricted is one tap away; the optimization list as a fallback. */
        fun openSettings(activity: Activity) {
            val intents = listOf(
                Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", activity.packageName, null)),
                Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS),
                Intent(Settings.ACTION_SETTINGS),
            )
            for (intent in intents) {
                try {
                    activity.startActivity(intent)
                    return
                } catch (e: ActivityNotFoundException) {
                    continue
                }
            }
        }
    }
}
