package app.organicmaps.widget.placepage

import android.Manifest.permission.ACCESS_COARSE_LOCATION
import android.Manifest.permission.ACCESS_FINE_LOCATION
import android.Manifest.permission.POST_NOTIFICATIONS
import android.app.NotificationManager
import android.location.Location
import android.location.LocationManager
import android.os.Build
import android.os.SystemClock
import android.view.inspector.WindowInspector
import android.widget.Button
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import androidx.test.core.app.ActivityScenario
import androidx.test.filters.SdkSuppress
import androidx.test.platform.app.InstrumentationRegistry
import app.organicmaps.MwmActivity
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.location.TrackRecordingService
import app.organicmaps.maplayer.MapButtonsController.MapButtons
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.bookmarks.data.Track
import app.organicmaps.sdk.location.TrackRecorder
import app.organicmaps.widget.TrackRecordingStartActivity
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

@SdkSuppress(minSdkVersion = 29)
class TrackRecordingDiscardTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val app = MwmApplication.from(instrumentation.targetContext)
    private lateinit var scenario: ActivityScenario<MwmActivity>
    private lateinit var delete: Button
    private var savedTrack: Track? = null

    @Before
    fun openConfirmation() {
        val permissions = mutableListOf(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) permissions.add(POST_NOTIFICATIONS)
        permissions.forEach { instrumentation.uiAutomation.grantRuntimePermission(app.packageName, it) }

        val ready = CountDownLatch(1)
        instrumentation.runOnMainSync {
            app.initOrganicMaps()
            app.mapsReadiness.runWhenReady(ready::countDown)
        }
        assertTrue("Maps did not become ready", ready.await(30, TimeUnit.SECONDS))
        scenario = ActivityScenario.launch(MwmActivity::class.java)
        scenario.onActivity { activity ->
            TrackRecordingService.startForegroundService(activity)
            val now = System.currentTimeMillis()
            for (i in 0..1) {
                val fix = Location(LocationManager.GPS_PROVIDER).apply {
                    time = now + i * 1000
                    elapsedRealtimeNanos = SystemClock.elapsedRealtimeNanos()
                    latitude = 47.0
                    longitude = 8.0 + i * 0.0002
                    accuracy = 1f
                    speed = 15f
                }
                app.locationHelper.onLocationChanged(fix)
            }
            assertTrue("The test recording must be saveable", TrackRecorder.nativeCanSaveTrackRecording())
            activity.onMapButtonClick(MapButtons.trackRecordingStatus)
            val fragments = activity.supportFragmentManager
            val page = fragments.findFragmentById(R.id.place_page_container_fragment) as PlacePageController
            page.onPlacePageButtonClick(PlacePageButtons.ButtonType.TRACK_RECORDING_DELETE)
            delete = WindowInspector.getGlobalWindowViews().mapNotNull { it.findViewById<Button>(android.R.id.button1) }
                .single { it.text == activity.getText(R.string.delete) }
            ViewModelProvider(activity)[PlacePageViewModel::class.java].mapObject.observe(activity) {
                if (it is Track) savedTrack = it
            }
        }
    }

    @After
    fun cleanUp() {
        if (!::scenario.isInitialized) return
        scenario.onActivity { activity ->
            if (TrackRecorder.nativeIsTrackRecordingEnabled()) activity.onTrackRecordingCancelled()
            savedTrack?.let { BookmarkManager.INSTANCE.deleteTrack(it.trackId) }
        }
        scenario.close()
    }

    @Test
    fun notificationStopAndSaveDismissesConfirmation() {
        val notifications = app.getSystemService(NotificationManager::class.java)
        val recordingId = TrackRecordingService.TRACK_REC_NOTIFICATION_ID
        val deadline = SystemClock.uptimeMillis() + 30_000
        while (true) {
            val notification = notifications.activeNotifications.find { it.id == recordingId }
            if (notification != null) {
                notification.notification.actions.first().actionIntent.send()
                break
            }
            assertTrue("Recording notification did not appear", SystemClock.uptimeMillis() < deadline)
            SystemClock.sleep(50)
        }
        awaitServiceStop()
        scenario.onActivity {
            assertNotNull("The saved track must be selected", savedTrack)
            assertFalse(delete.isAttachedToWindow)
        }
    }

    @Test
    fun deleteDiscardsTheCurrentRecording() {
        scenario.onActivity { assertTrue(delete.performClick()) }
        instrumentation.waitForIdleSync()
        scenario.onActivity {
            assertFalse(TrackRecorder.nativeIsTrackRecordingEnabled())
            assertNull("Discarding must not save a track", savedTrack)
        }
    }

    @Test
    fun queuedDeleteCannotDiscardNextRecording() {
        scenario.onActivity {
            // AlertController posts the click. Finish and replace the recording in this same main-loop turn.
            assertTrue(delete.performClick())
            assertTrue(TrackRecorder.saveAndStop())
            assertFalse(delete.isAttachedToWindow)
            TrackRecorder.nativeStartTrackRecording()
        }
        instrumentation.waitForIdleSync()
        scenario.onActivity {
            assertTrue("The old Delete stopped the new recording", TrackRecorder.nativeIsTrackRecordingEnabled())
        }
    }

    @Test
    fun widgetRestartWhileMapIsStoppedInvalidatesConfirmation() {
        scenario.moveToState(Lifecycle.State.CREATED)
        TrackRecordingService.stopService(app)
        awaitServiceStop()
        ActivityScenario.launch(TrackRecordingStartActivity::class.java).close()
        scenario.moveToState(Lifecycle.State.RESUMED)
        scenario.onActivity {
            assertTrue("The widget must have restarted recording", TrackRecorder.nativeIsTrackRecordingEnabled())
            assertFalse("The previous recording's confirmation is still attached", delete.isAttachedToWindow)
        }
    }

    private fun awaitServiceStop() {
        val stopped = CountDownLatch(1)
        val observer = Observer<Boolean> { if (it == false) stopped.countDown() }
        val recording = TrackRecordingService.isRecording()
        instrumentation.runOnMainSync { recording.observeForever(observer) }
        try {
            assertTrue("Recording service did not stop", stopped.await(30, TimeUnit.SECONDS))
        } finally {
            instrumentation.runOnMainSync { recording.removeObserver(observer) }
        }
    }
}
