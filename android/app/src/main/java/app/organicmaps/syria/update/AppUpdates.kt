package app.organicmaps.syria.update

import android.app.Activity
import android.content.Context
import android.os.Build

/**
 * Tells users about a new app version. An app installed from Google Play uses Play's in-app update (the Google
 * build only, [PlayInAppUpdate]); any other install (Huawei build, APK from the website, development builds) reads
 * the latest release from our server ([ServerUpdateCheck]). Map data is not touched: maps update on their own.
 */
object AppUpdates {
    private const val PLAY_STORE = "com.android.vending"

    /** Once the map is ready: looks for a new version, at most every few hours. */
    @JvmStatic
    fun check(activity: Activity) {
        if (installedFromPlay(activity) && PlayInAppUpdate.check(activity)) return
        ServerUpdateCheck.check(activity)
    }

    /** On every resume: an update Play downloaded in the background is offered for install. */
    @JvmStatic
    fun onResume(activity: Activity) {
        if (installedFromPlay(activity)) PlayInAppUpdate.onResume(activity)
    }

    private fun installedFromPlay(context: Context): Boolean {
        val installer = try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                context.packageManager.getInstallSourceInfo(context.packageName).installingPackageName
            } else {
                @Suppress("DEPRECATION")
                context.packageManager.getInstallerPackageName(context.packageName)
            }
        } catch (e: Exception) {
            null
        }
        return installer == PLAY_STORE
    }
}
