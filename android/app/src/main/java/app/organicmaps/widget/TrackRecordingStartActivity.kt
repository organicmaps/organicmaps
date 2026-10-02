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
 * Starts track recording from the home screen widget without showing the map. As a visible activity it brings the app
 * to the foreground, so Android lets it start the location foreground service.
 */
class TrackRecordingStartActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // SplashActivity is bypassed here; an already running Android Auto session keeps the car display.
        MwmApplication.from(this).displayManager.init(DisplayType.Device)
        if (!startTrackRecording()) {
            // The app asks for the location permission or tells about the unavailable storage.
            val intent =
                Intent(this, MwmActivity::class.java)
                    .putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(intent)
        }
        finish()
    }

    private fun startTrackRecording(): Boolean {
        // Checked before initializing the core, so that a cold MwmActivity starts through SplashActivity, which also
        // prepares the first launch.
        if (!LocationUtils.checkFineLocationPermission(this)) {
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
