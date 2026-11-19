package app.organicmaps.syria.update

import android.app.Activity

/** Builds without Google Play (Huawei, F-Droid, web): updates are announced by [ServerUpdateCheck] instead. */
object PlayInAppUpdate {
    fun check(activity: Activity): Boolean = false

    fun onResume(activity: Activity) = Unit
}
