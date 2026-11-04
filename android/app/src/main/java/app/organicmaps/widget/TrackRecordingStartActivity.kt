package app.organicmaps.widget

import android.Manifest.permission.ACCESS_COARSE_LOCATION
import android.Manifest.permission.ACCESS_FINE_LOCATION
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import app.organicmaps.MwmActivity
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.location.TrackRecordingService
import app.organicmaps.sdk.display.DisplayType
import app.organicmaps.sdk.location.LocationUtils
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.util.BatteryOptimizationHelper
import java.io.IOException

/**
 * Starts recording from the widget without taking the map away from Android Auto.
 * Android allows starting a location foreground service only from the foreground.
 */
class TrackRecordingStartActivity : AppCompatActivity() {
    private val batterySettings = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
        continueRecordingStart()
    }
    private val locationPermission = registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {
        if (LocationUtils.checkFineLocationPermission(this)) {
            continueRecordingStart()
        } else {
            Toast.makeText(this, R.string.precise_location_is_disabled_long_text, Toast.LENGTH_LONG).show()
            finishAndRemoveTask()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Normally done by SplashActivity, no-op if already initialized.
        MwmApplication.from(this).displayManager.init(DisplayType.Device)
        supportFragmentManager.setFragmentResultListener(BatteryOptimizationHelper.REQUEST_KEY, this) { _, result ->
            when (val action = result.getInt(BatteryOptimizationHelper.BUNDLE_KEY_ACTION)) {
                BatteryOptimizationHelper.ACTION_GO_TO_SETTINGS -> {
                    if (!BatteryOptimizationHelper.openSettings(this, batterySettings)) {
                        continueRecordingStart()
                    }
                }

                BatteryOptimizationHelper.ACTION_CONTINUE -> continueRecordingStart()

                else -> error("Unknown battery warning action: $action")
            }
        }
        // A restored dialog or Activity Result resumes the unfinished request.
        if (savedInstanceState == null) {
            if (BatteryOptimizationHelper.shouldShowWarning(this)) {
                BatteryOptimizationHelper.showOptimizationDialog(this)
            } else {
                continueRecordingStart()
            }
        }
    }

    private fun continueRecordingStart() {
        if (!LocationUtils.checkFineLocationPermission(this)) {
            locationPermission.launch(arrayOf(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION))
            return
        }
        val app = MwmApplication.from(this)
        if (!LocationUtils.areLocationServicesTurnedOn(this) && !app.displayManager.isCarDisplayUsed) {
            openMap()
            return
        }

        if (!app.organicMaps.arePlatformAndCoreInitialized()) {
            try {
                app.initOrganicMaps(null)
            } catch (e: IOException) {
                Logger.e(TAG, "Failed to initialize the app", e)
                openMap()
                return
            }
        }

        TrackRecordingService.startForegroundService(this)
        finishAndRemoveTask()
    }

    private fun openMap() {
        startActivity(
            Intent(this, MwmActivity::class.java)
                .putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        )
        finishAndRemoveTask()
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
