package app.organicmaps.widget

import android.app.Activity
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import app.organicmaps.MwmActivity
import app.organicmaps.MwmApplication
import app.organicmaps.location.TrackRecordingService
import app.organicmaps.sdk.display.DisplayType
import app.organicmaps.sdk.location.LocationUtils
import app.organicmaps.sdk.util.log.Logger
import java.io.IOException

/**
 * Starts track recording from the widget without opening the map.
 * Android allows starting a location foreground service only from the foreground.
 */
class TrackRecordingStartActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Normally done by SplashActivity, no-op if already initialized.
        MwmApplication.from(this).displayManager.init(DisplayType.Device)
        if (!startTrackRecording()) {
            // Let the map ask for permissions or report errors.
            val intent =
                Intent(this, MwmActivity::class.java)
                    .putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(intent)
        }
        finish()
    }

    private fun startTrackRecording(): Boolean {
        // Before core init, so that a cold start still goes through SplashActivity.
        if (!LocationUtils.checkFineLocationPermission(this) || !LocationUtils.areLocationServicesTurnedOn(this)) {
            return false
        }

        val app = MwmApplication.from(this)
        if (!app.organicMaps.arePlatformAndCoreInitialized()) {
            try {
                app.initOrganicMaps(null)
            } catch (e: IOException) {
                Logger.e(TAG, "Failed to initialize the app", e)
                return false
            }
        }

        TrackRecordingService.startForegroundService(this)
        return true
    }

    companion object {
        private val TAG = TrackRecordingStartActivity::class.java.simpleName

        fun createPendingIntent(context: Context): PendingIntent {
            val immutable = if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) 0 else PendingIntent.FLAG_IMMUTABLE
            val flags = PendingIntent.FLAG_UPDATE_CURRENT or immutable
            val intent = Intent(context, TrackRecordingStartActivity::class.java)
            return PendingIntent.getActivity(context, 0, intent, flags)
        }
    }
}
