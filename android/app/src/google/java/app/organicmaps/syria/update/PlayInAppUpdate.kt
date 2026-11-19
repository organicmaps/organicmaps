package app.organicmaps.syria.update

import android.app.Activity
import app.organicmaps.R
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.play.core.appupdate.AppUpdateInfo
import com.google.android.play.core.appupdate.AppUpdateManager
import com.google.android.play.core.appupdate.AppUpdateManagerFactory
import com.google.android.play.core.appupdate.AppUpdateOptions
import com.google.android.play.core.install.InstallStateUpdatedListener
import com.google.android.play.core.install.model.AppUpdateType
import com.google.android.play.core.install.model.InstallStatus
import com.google.android.play.core.install.model.UpdateAvailability
import java.lang.ref.WeakReference

/**
 * Google Play in-app update for an app installed from Play: Play's own "Update available" sheet, then the download
 * runs in the background and the user is asked to restart once it is ready. A release published with update
 * priority 4 or 5 in Play Console uses the full-screen flow, which must be completed before using the app.
 */
object PlayInAppUpdate {
    private const val REQUEST_CODE = 0x5359
    private const val IMMEDIATE_PRIORITY = 4

    private var checkedThisRun = false
    private var manager: AppUpdateManager? = null
    private var resumed = WeakReference<Activity>(null)

    private val installListener = InstallStateUpdatedListener { state ->
        if (state.installStatus() == InstallStatus.DOWNLOADED) resumed.get()?.let { askToRestart(it) }
    }

    /** Asks Play once per app start; falls back to our server if Play cannot answer. */
    fun check(activity: Activity): Boolean {
        if (checkedThisRun) return true
        checkedThisRun = true
        val manager = manager(activity)
        manager.appUpdateInfo
            .addOnSuccessListener { info ->
                if (activity.isFinishing || activity.isDestroyed) return@addOnSuccessListener
                when {
                    info.installStatus() == InstallStatus.DOWNLOADED -> askToRestart(activity)
                    info.updateAvailability() != UpdateAvailability.UPDATE_AVAILABLE -> Unit
                    info.updatePriority() >= IMMEDIATE_PRIORITY && info.isUpdateTypeAllowed(AppUpdateType.IMMEDIATE) ->
                        start(activity, manager, info, AppUpdateType.IMMEDIATE)
                    info.isUpdateTypeAllowed(AppUpdateType.FLEXIBLE) -> {
                        manager.registerListener(installListener)
                        start(activity, manager, info, AppUpdateType.FLEXIBLE)
                    }
                }
            }
            .addOnFailureListener { ServerUpdateCheck.check(activity) }
        return true
    }

    fun onResume(activity: Activity) {
        resumed = WeakReference(activity)
        val manager = manager ?: return
        manager.appUpdateInfo.addOnSuccessListener { info ->
            if (activity.isFinishing || activity.isDestroyed) return@addOnSuccessListener
            when {
                info.installStatus() == InstallStatus.DOWNLOADED -> askToRestart(activity)
                // A required update that was left half-way continues.
                info.updateAvailability() == UpdateAvailability.DEVELOPER_TRIGGERED_UPDATE_IN_PROGRESS ->
                    start(activity, manager, info, AppUpdateType.IMMEDIATE)
            }
        }
    }

    private fun manager(activity: Activity): AppUpdateManager =
        manager ?: AppUpdateManagerFactory.create(activity.applicationContext).also { manager = it }

    private fun start(
        activity: Activity,
        manager: AppUpdateManager,
        info: AppUpdateInfo,
        type: Int,
    ) {
        manager.startUpdateFlowForResult(info, activity, AppUpdateOptions.defaultOptions(type), REQUEST_CODE)
    }

    private fun askToRestart(activity: Activity) {
        MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.syria_update_downloaded_title)
            .setMessage(R.string.syria_update_downloaded_message)
            .setPositiveButton(R.string.syria_update_restart) { _, _ ->
                manager?.unregisterListener(installListener)
                manager?.completeUpdate()
            }
            .setNegativeButton(R.string.syria_update_later, null)
            .show()
    }
}
