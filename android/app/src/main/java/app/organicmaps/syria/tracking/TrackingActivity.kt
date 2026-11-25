package app.organicmaps.syria.tracking

import android.Manifest
import android.animation.ObjectAnimator
import android.animation.ValueAnimator
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.format.DateFormat
import android.text.format.DateUtils
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.LinearInterpolator
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.widget.Toolbar
import androidx.core.content.ContextCompat
import androidx.core.graphics.ColorUtils
import androidx.core.view.ViewCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity
import app.organicmaps.syria.NarrationDialog
import app.organicmaps.syria.SyriaFormat
import app.organicmaps.syria.SyriaMapButtons
import app.organicmaps.syria.SyriaPrefs
import app.organicmaps.syria.SyriaStatus
import app.organicmaps.syria.SyriaTutorial
import app.organicmaps.syria.account.LoginActivity
import app.organicmaps.util.UiUtils
import app.organicmaps.util.Utils
import app.organicmaps.util.WindowInsetUtils
import com.google.android.material.button.MaterialButton
import com.google.android.material.card.MaterialCardView
import com.google.android.material.color.MaterialColors
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.progressindicator.LinearProgressIndicator
import java.util.Date

/** Start/pause/resume/stop screen with live distance and time, the sync status and the recordings. */
class TrackingActivity : BaseMwmFragmentActivity() {
    private val handler = Handler(Looper.getMainLooper())
    private var snapshot: TrackingEngine.Snapshot? = null
    private var snapshotReceivedAt = 0L
    private var page = 0
    private var syncSpin: ObjectAnimator? = null
    private var manualSyncAt = 0L
    private lateinit var syncLastDefaultColors: ColorStateList

    private lateinit var status: TextView
    private lateinit var light: ImageView
    private lateinit var elapsed: TextView
    private lateinit var distance: TextView
    private lateinit var moving: TextView
    private lateinit var points: TextView
    private lateinit var startButton: Button
    private lateinit var pauseButton: Button
    private lateinit var stopButton: Button
    private lateinit var root: View
    private lateinit var syncCard: MaterialCardView
    private lateinit var batteryCard: MaterialCardView
    private lateinit var batteryIcon: ImageView
    private lateinit var batteryStatus: TextView
    private lateinit var batteryHint: TextView
    private lateinit var batteryFix: MaterialButton
    private lateinit var syncIcon: ImageView
    private lateinit var syncStatus: TextView
    private lateinit var syncLast: TextView
    private lateinit var syncNow: MaterialButton
    private lateinit var liveButton: MaterialButton
    private lateinit var syncProgress: LinearProgressIndicator
    private lateinit var historyTitle: TextView
    private lateinit var historyEmpty: TextView
    private lateinit var clearAll: View
    private lateinit var pager: View
    private lateinit var pageLabel: TextView
    private lateinit var pageRange: TextView
    private lateinit var pagePrev: View
    private lateinit var pageNext: View
    private val adapter = SessionAdapter()

    // The answer only lists the permissions that were asked: often just notifications, when location was
    // granted before. So the real permission state decides, not the answer.
    private val permissionRequest =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { onPermissionsAnswered() }

    private val livePermissionRequest =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {
            if (granted(Manifest.permission.ACCESS_FINE_LOCATION)) {
                enableLive()
            } else {
                Toast.makeText(this, R.string.precise_location_is_disabled_long_text, Toast.LENGTH_LONG).show()
            }
        }

    private val ticker = object : Runnable {
        override fun run() {
            renderLive()
            handler.postDelayed(this, 1000)
        }
    }

    override fun getContentLayoutResId(): Int = R.layout.activity_syria_tracking

    override fun onSafeCreate(savedInstanceState: Bundle?) {
        super.onSafeCreate(savedInstanceState)
        val drivingGuide = intent.getBooleanExtra(EXTRA_DRIVING_GUIDE, false)
        if (!SyriaPrefs.isLoggedIn(this)) {
            // The login screen comes back here, with the guide when the driving reminder was tapped.
            startActivity(Intent(this, LoginActivity::class.java).putExtra(EXTRA_DRIVING_GUIDE, drivingGuide))
            finish()
            return
        }
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        toolbar.setTitle(R.string.syria_tracking_title)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { finish() }
        // Instructions: by themselves on the first visit, then with the "?" icon. Not over the driving guide.
        NarrationDialog.attach(this, toolbar, NarrationDialog.RECORDING, savedInstanceState == null && !drivingGuide)
        // Like BaseToolbarActivity: keeps the toolbar (and its back arrow) below the status bar.
        ViewCompat.setOnApplyWindowInsetsListener(toolbar, WindowInsetUtils.PaddingInsetsListener.excludeBottom())
        ViewCompat.setOnApplyWindowInsetsListener(
            findViewById(R.id.tracking_scroll),
            WindowInsetUtils.PaddingInsetsListener.excludeTop(),
        )

        root = findViewById(R.id.tracking_root)
        status = findViewById(R.id.tracking_status)
        light = findViewById(R.id.tracking_light)
        elapsed = findViewById(R.id.tracking_elapsed)
        distance = findViewById(R.id.tracking_distance)
        moving = findViewById(R.id.tracking_moving)
        points = findViewById(R.id.tracking_points)
        startButton = findViewById(R.id.tracking_start)
        pauseButton = findViewById(R.id.tracking_pause)
        stopButton = findViewById(R.id.tracking_stop)
        syncCard = findViewById(R.id.tracking_sync_card)
        batteryCard = findViewById(R.id.tracking_battery_card)
        batteryIcon = findViewById(R.id.tracking_battery_icon)
        batteryStatus = findViewById(R.id.tracking_battery_status)
        batteryHint = findViewById(R.id.tracking_battery_hint)
        batteryFix = findViewById(R.id.tracking_battery_fix)
        batteryFix.setOnClickListener { BackgroundMode.openSettings(this) }
        syncIcon = findViewById(R.id.tracking_sync_icon)
        syncStatus = findViewById(R.id.tracking_sync_status)
        syncLast = findViewById(R.id.tracking_sync_last)
        syncLastDefaultColors = syncLast.textColors
        syncNow = findViewById(R.id.tracking_sync_now)
        liveButton = findViewById(R.id.tracking_live)
        liveButton.setOnClickListener { onLiveClicked() }
        // Offered with internet; while it is on, it stays visible offline too so it can be turned off.
        SyriaStatus.online().observe(this) { renderLiveToggle() }
        LiveLocationService.running().observe(this) { renderLiveToggle() }
        syncProgress = findViewById(R.id.tracking_sync_progress)
        historyTitle = findViewById(R.id.tracking_history_title)
        historyEmpty = findViewById(R.id.tracking_history_empty)
        clearAll = findViewById(R.id.tracking_clear_all)
        clearAll.setOnClickListener { confirmClearAll() }
        pager = findViewById(R.id.tracking_pager)
        pageLabel = findViewById(R.id.tracking_page_label)
        pageRange = findViewById(R.id.tracking_page_range)
        pagePrev = findViewById(R.id.tracking_page_prev)
        pageNext = findViewById(R.id.tracking_page_next)

        startButton.setOnClickListener { requestPermissionsThenStart() }
        pauseButton.setOnClickListener {
            when (snapshot?.status) {
                TrackingEngine.Status.ACTIVE -> ActivityTrackingService.pause(this)
                TrackingEngine.Status.PAUSED -> ActivityTrackingService.resume(this)
                else -> Unit
            }
        }
        stopButton.setOnClickListener { confirmStop() }
        syncNow.setOnClickListener { onSyncNow() }
        pagePrev.setOnClickListener {
            page--
            loadHistory()
        }
        pageNext.setOnClickListener {
            page++
            loadHistory()
        }

        val history = findViewById<RecyclerView>(R.id.tracking_history)
        history.layoutManager = LinearLayoutManager(this)
        history.adapter = adapter

        ActivityTrackingService.state().observe(this) {
            val statusChanged = it?.status != snapshot?.status
            snapshot = it
            snapshotReceivedAt = System.currentTimeMillis()
            render()
            // The current recording is on the first page; other pages only change when one starts or stops.
            if (page == 0 || statusChanged) loadHistory()
        }
        SyriaStatus.sync().observe(this) {
            renderSync(it)
            loadHistory()
        }
        // The state LiveData may have no value yet when nothing was recorded since the app started.
        render()
        SyriaStatus.refresh(this)
        ActivityTrackingService.resumeIfNeeded(this)
        // Once, not again after a rotation.
        if (drivingGuide && savedInstanceState == null) root.post { showDrivingGuide() }
    }

    /** From the "Are you driving?" reminder: spotlights Start, then live location, skipping what is already on. */
    private fun showDrivingGuide() {
        if (isFinishing) return
        val steps = ArrayList<Pair<SyriaTutorial.Step, View>>()
        if ((snapshot?.status ?: TrackingEngine.Status.IDLE) == TrackingEngine.Status.IDLE) {
            val step = SyriaTutorial.Step(
                startButton.id,
                R.string.syria_guide_start_title,
                R.string.syria_guide_start_text,
                tapTarget = true,
            )
            steps.add(step to startButton)
        }
        if (!SyriaPrefs.isLiveEnabled(this)) {
            val step = SyriaTutorial.Step(
                liveButton.id,
                R.string.syria_guide_live_title,
                R.string.syria_guide_live_text,
                tapTarget = true,
            )
            steps.add(step to liveButton)
        }
        SyriaTutorial.spotlight(this, steps, showDontShow = false) { _, _ -> }
    }

    override fun onResume() {
        super.onResume()
        // Nothing to drive when onSafeCreate() bounced to the login screen.
        if (!::status.isInitialized) return
        handler.post(ticker)
        loadHistory()
        // Also when coming back from the battery settings.
        renderBattery()
        // Live location that was on when the app was closed starts again here.
        LiveLocationService.startIfEnabled(this)
    }

    override fun onPause() {
        handler.removeCallbacks(ticker)
        super.onPause()
    }

    override fun onSafeDestroy() {
        syncSpin?.cancel()
        super.onSafeDestroy()
    }

    private fun onSyncNow() {
        manualSyncAt = System.currentTimeMillis()
        // Queued either way: WorkManager starts it as soon as the network is back.
        SyncWorker.syncNowManually(this)
        if (SyriaStatus.online().value == false) {
            Toast.makeText(this, R.string.syria_sync_offline, Toast.LENGTH_LONG).show()
        }
    }

    private fun onLiveClicked() {
        if (SyriaPrefs.isLiveEnabled(this)) {
            LiveLocationService.disable(this)
        } else if (granted(Manifest.permission.ACCESS_FINE_LOCATION)) {
            enableLive()
        } else {
            livePermissionRequest.launch(
                arrayOf(Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission.ACCESS_FINE_LOCATION),
            )
        }
        renderLiveToggle()
    }

    private fun enableLive() {
        LiveLocationService.enable(this)
        Toast.makeText(this, R.string.syria_live_enabled, Toast.LENGTH_LONG).show()
        renderLiveToggle()
    }

    /** Live location toggle: filled green while on (like the live badge on the map), outlined when off. */
    private fun renderLiveToggle() {
        val online = SyriaStatus.online().value == true
        val on = SyriaPrefs.isLiveEnabled(this)
        liveButton.visibility = if (online || on) View.VISIBLE else View.GONE
        liveButton.setText(
            when {
                !on -> R.string.syria_live_off
                online -> R.string.syria_live_on
                else -> R.string.syria_live_paused
            },
        )
        val fill = ContextCompat.getColor(this, R.color.syria_btn_start)
        val onFill = ContextCompat.getColor(this, R.color.syria_btn_on_start)
        val text = MaterialColors.getColor(liveButton, android.R.attr.textColorPrimary)
        liveButton.backgroundTintList = ColorStateList.valueOf(if (on) fill else Color.TRANSPARENT)
        liveButton.setTextColor(if (on) onFill else text)
        liveButton.iconTint = ColorStateList.valueOf(if (on) onFill else text)
        liveButton.strokeColor = ColorStateList.valueOf(if (on) fill else text)
    }

    private fun requestPermissionsThenStart() {
        val missing = ArrayList<String>()
        if (!granted(Manifest.permission.ACCESS_FINE_LOCATION)) {
            missing.add(Manifest.permission.ACCESS_COARSE_LOCATION)
            missing.add(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        // Notifications only show the recording in the status bar; it records without them. Asked once.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            !granted(Manifest.permission.POST_NOTIFICATIONS) &&
            !SyriaPrefs.isNotificationPermissionAsked(this)
        ) {
            SyriaPrefs.markNotificationPermissionAsked(this)
            missing.add(Manifest.permission.POST_NOTIFICATIONS)
        }
        if (missing.isEmpty()) startTracking() else permissionRequest.launch(missing.toTypedArray())
    }

    private fun onPermissionsAnswered() {
        when {
            granted(Manifest.permission.ACCESS_FINE_LOCATION) -> startTracking()
            // Approximate location (about 2 km) cannot record a route.
            granted(Manifest.permission.ACCESS_COARSE_LOCATION) ->
                Toast.makeText(this, R.string.precise_location_is_disabled_long_text, Toast.LENGTH_LONG).show()
            else -> Toast.makeText(this, R.string.location_is_disabled_long_text, Toast.LENGTH_LONG).show()
        }
    }

    private fun granted(permission: String): Boolean =
        ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_GRANTED

    private fun startTracking() {
        ActivityTrackingService.start(this)
        Utils.showBatteryOptimizationWarningIfNeeded(this)
    }

    private fun confirmStop() {
        MaterialAlertDialogBuilder(this)
            .setTitle(R.string.syria_stop)
            .setMessage(R.string.syria_stop_confirm)
            .setNegativeButton(R.string.syria_cancel, null)
            .setPositiveButton(R.string.syria_stop) { _, _ -> ActivityTrackingService.stop(this) }
            .show()
    }

    private fun render() {
        val s = snapshot
        val st = s?.status ?: TrackingEngine.Status.IDLE
        status.setText(
            when (st) {
                TrackingEngine.Status.ACTIVE -> R.string.syria_status_active
                TrackingEngine.Status.PAUSED -> R.string.syria_status_paused
                TrackingEngine.Status.IDLE -> R.string.syria_status_idle
            },
        )
        startButton.visibility = if (st == TrackingEngine.Status.IDLE) View.VISIBLE else View.GONE
        pauseButton.visibility = if (st == TrackingEngine.Status.IDLE) View.GONE else View.VISIBLE
        stopButton.visibility = pauseButton.visibility
        pauseButton.setText(if (st == TrackingEngine.Status.ACTIVE) R.string.syria_pause else R.string.syria_resume)
        // Same light as the map button: green while recording, red otherwise.
        val recording = SyriaMapButtons.isRecording(st)
        light.setImageResource(if (recording) R.drawable.ic_syria_lamp_green else R.drawable.ic_syria_lamp_red)
        val lamp = ContextCompat.getColor(this, if (recording) R.color.syria_traffic_green else R.color.syria_traffic_red)
        val background = MaterialColors.getColor(root, android.R.attr.colorBackground)
        root.setBackgroundColor(ColorUtils.compositeColors(ColorUtils.setAlphaComponent(lamp, SCREEN_ALPHA), background))
        distance.text = ActivityTrackingService.formatKm(this, s?.distanceM ?: 0.0)
        points.text = (s?.pointCount ?: 0).toString()
        renderLive()
    }

    private fun renderLive() {
        val s = snapshot
        if (s == null || s.status == TrackingEngine.Status.IDLE) {
            elapsed.text = SyriaFormat.clock(this, 0)
            moving.text = SyriaFormat.clock(this, 0)
            return
        }
        val now = System.currentTimeMillis()
        elapsed.text = SyriaFormat.clock(this, (now - s.startedAtMs) / 1000)
        val extra = if (s.status == TrackingEngine.Status.ACTIVE) (now - snapshotReceivedAt) / 1000 else 0L
        moving.text = SyriaFormat.clock(this, s.movingTimeS + extra)
    }

    /** The sync colours stay inside the sync card; the screen itself follows the traffic light. */
    private fun renderBattery() {
        val mode = BackgroundMode.current(this)
        val color = ContextCompat.getColor(
            this,
            when (mode) {
                BackgroundMode.UNRESTRICTED -> R.color.syria_sync_green
                BackgroundMode.OPTIMIZED -> R.color.syria_sync_orange
                BackgroundMode.RESTRICTED -> R.color.syria_sync_red
            },
        )
        batteryCard.setCardBackgroundColor(ColorUtils.setAlphaComponent(color, CARD_ALPHA))
        batteryIcon.imageTintList = ColorStateList.valueOf(color)
        batteryStatus.setText(
            when (mode) {
                BackgroundMode.UNRESTRICTED -> R.string.syria_battery_unrestricted
                BackgroundMode.OPTIMIZED -> R.string.syria_battery_optimized
                BackgroundMode.RESTRICTED -> R.string.syria_battery_restricted
            },
        )
        batteryHint.setText(
            when (mode) {
                BackgroundMode.UNRESTRICTED -> R.string.syria_battery_unrestricted_hint
                BackgroundMode.OPTIMIZED -> R.string.syria_battery_optimized_hint
                BackgroundMode.RESTRICTED -> R.string.syria_battery_restricted_hint
            },
        )
        // In the status colour: the default outline is hard to see on the tinted card in dark mode.
        batteryFix.setTextColor(color)
        batteryFix.strokeColor = ColorStateList.valueOf(color)
        batteryFix.visibility = if (mode == BackgroundMode.UNRESTRICTED) View.GONE else View.VISIBLE
    }

    private fun renderSync(sync: SyriaStatus.Sync) {
        val lastError = SyriaPrefs.lastSyncError(this)
        val unreachable =
            sync.state == SyriaStatus.SyncState.FAILED && lastError == SyriaPrefs.SYNC_ERROR_UNREACHABLE
        // A "Sync now" tap that ended with the server out of reach also gets a message, once.
        if (unreachable && manualSyncAt != 0L && SyriaPrefs.lastSyncResultAt(this) >= manualSyncAt) {
            manualSyncAt = 0L
            Toast.makeText(this, R.string.syria_sync_unreachable_toast, Toast.LENGTH_LONG).show()
        } else if (sync.state == SyriaStatus.SyncState.SYNCED) {
            manualSyncAt = 0L
        }
        val color = ContextCompat.getColor(this, SyriaMapButtons.colorOf(sync.state))
        syncCard.setCardBackgroundColor(ColorUtils.setAlphaComponent(color, CARD_ALPHA))
        syncIcon.imageTintList = ColorStateList.valueOf(color)
        syncIcon.setImageResource(
            when (sync.state) {
                SyriaStatus.SyncState.SYNCED -> R.drawable.ic_syria_sync_done
                SyriaStatus.SyncState.SYNCING -> R.drawable.ic_syria_sync
                SyriaStatus.SyncState.PENDING -> R.drawable.ic_syria_sync_pending
                SyriaStatus.SyncState.FAILED, SyriaStatus.SyncState.SIGNED_OUT -> R.drawable.ic_syria_sync_failed
            },
        )
        syncStatus.text = when (sync.state) {
            SyriaStatus.SyncState.SYNCED -> getString(R.string.syria_synced_all)
            SyriaStatus.SyncState.PENDING -> getString(R.string.syria_unsynced_format, sync.unsynced)
            SyriaStatus.SyncState.SYNCING ->
                if (sync.total > 0) {
                    getString(R.string.syria_sync_progress, sync.done, sync.total)
                } else {
                    getString(R.string.syria_syncing)
                }
            SyriaStatus.SyncState.FAILED ->
                getString(if (unreachable) R.string.syria_sync_unreachable else R.string.syria_sync_failed)
            SyriaStatus.SyncState.SIGNED_OUT -> getString(R.string.syria_sync_signed_out)
        }
        val lastSync = SyriaPrefs.lastSyncAt(this)
        // Highlighted in the status colour: syncing needs the internet, the data is not lost.
        if (unreachable) {
            syncLast.setTextColor(color)
            syncLast.setTypeface(null, Typeface.BOLD)
        } else {
            syncLast.setTextColor(syncLastDefaultColors)
            syncLast.setTypeface(null, Typeface.NORMAL)
        }
        syncLast.text = if (unreachable) {
            getString(R.string.syria_sync_internet_needed)
        } else if (sync.state == SyriaStatus.SyncState.FAILED && lastError != null) {
            getString(R.string.syria_sync_failed_reason, lastError)
        } else if (lastSync == 0L) {
            getString(R.string.syria_never_synced)
        } else {
            getString(
                R.string.syria_last_synced,
                DateUtils.getRelativeTimeSpanString(lastSync, System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS),
            )
        }
        val syncing = sync.state == SyriaStatus.SyncState.SYNCING
        syncNow.isEnabled = !syncing
        spinSyncIcon(syncing)
        syncProgress.visibility = if (syncing) View.VISIBLE else View.GONE
        if (syncing) {
            if (sync.total > 0) {
                syncProgress.isIndeterminate = false
                syncProgress.max = sync.total
                syncProgress.setProgressCompat(sync.done, true)
            } else {
                syncProgress.isIndeterminate = true
            }
        }
    }

    /** Rotates the status icon while an upload runs. */
    private fun spinSyncIcon(spin: Boolean) {
        if (spin) {
            if (syncSpin != null) return
            // Counter-clockwise like the arrows of the sync icon.
            syncSpin = ObjectAnimator.ofFloat(syncIcon, View.ROTATION, 360f, 0f).apply {
                duration = SPIN_MS
                repeatCount = ValueAnimator.INFINITE
                interpolator = LinearInterpolator()
                start()
            }
        } else {
            syncSpin?.cancel()
            syncSpin = null
            syncIcon.rotation = 0f
        }
    }

    private fun loadHistory() {
        Thread {
            val db = TrackingDb.get(this)
            val total = db.countSessions()
            val pages = maxOf(1, (total + PAGE_SIZE - 1) / PAGE_SIZE)
            val current = page.coerceIn(0, pages - 1)
            val sessions = db.listSessions(PAGE_SIZE, current * PAGE_SIZE)
            val hasFinished = sessions.any { it.status == TrackingDb.STATUS_STOPPED } || total > sessions.size
            runOnUiThread {
                if (isFinishing) return@runOnUiThread
                page = current
                adapter.submit(sessions)
                historyTitle.text =
                    if (total == 0) getString(R.string.syria_history) else getString(R.string.syria_history_count, total)
                historyEmpty.visibility = if (total == 0) View.VISIBLE else View.GONE
                clearAll.visibility = if (hasFinished) View.VISIBLE else View.GONE
                pager.visibility = if (total > 0) View.VISIBLE else View.GONE
                pageLabel.text = getString(R.string.syria_page_format, current + 1, pages)
                val first = current * PAGE_SIZE + 1
                pageRange.text = getString(R.string.syria_page_range, first, first + sessions.size - 1, total)
                // A single page needs no buttons, only where the list starts and ends.
                val buttons = if (pages > 1) View.VISIBLE else View.INVISIBLE
                pagePrev.visibility = buttons
                pageNext.visibility = buttons
                pagePrev.isEnabled = current > 0
                pagePrev.alpha = if (current > 0) 1f else DISABLED_ALPHA
                pageNext.isEnabled = current < pages - 1
                pageNext.alpha = if (current < pages - 1) 1f else DISABLED_ALPHA
            }
        }.start()
    }

    /** Deletes every finished recording from the device after a confirmation that says what will be lost. */
    private fun confirmClearAll() {
        Thread {
            val db = TrackingDb.get(this)
            val finished = db.countSessions() - (if (db.getOpenSession() != null) 1 else 0)
            val unsynced = db.countUnsyncedStoppedSessions()
            runOnUiThread {
                if (isFinishing || finished <= 0) return@runOnUiThread
                val message = buildString {
                    append(resources.getQuantityString(R.plurals.syria_history_clear_all_confirm, finished, finished))
                    if (unsynced > 0) {
                        append("\n\n")
                        append(resources.getQuantityString(R.plurals.syria_history_clear_unsynced, unsynced, unsynced))
                    }
                }
                MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.syria_history_clear_all_title)
                    .setMessage(message)
                    .setNegativeButton(R.string.syria_cancel, null)
                    .setPositiveButton(R.string.syria_history_clear_all) { _, _ -> clearAllRecordings() }
                    .show()
            }
        }.start()
    }

    private fun clearAllRecordings() {
        Thread {
            val deleted = TrackingDb.get(this).deleteStoppedSessions()
            runOnUiThread {
                if (isFinishing) return@runOnUiThread
                page = 0
                loadHistory()
                Toast.makeText(
                    this,
                    resources.getQuantityString(R.plurals.syria_history_cleared, deleted, deleted),
                    Toast.LENGTH_SHORT,
                ).show()
            }
        }.start()
    }

    private fun totalSeconds(session: Session): Long {
        val end = session.endedAt ?: System.currentTimeMillis()
        return (end - session.startedAt).coerceAtLeast(0) / 1000
    }

    private fun startedText(session: Session): String {
        val date = Date(session.startedAt)
        return DateFormat.getMediumDateFormat(this).format(date) + " " + DateFormat.getTimeFormat(this).format(date)
    }

    private fun showSession(session: Session) {
        val km = ActivityTrackingService.formatKm(this, session.distanceM)
        val syncState =
            getString(if (session.synced) R.string.syria_sync_status_synced else R.string.syria_sync_status_pending)
        val message = buildString {
            append(getString(R.string.syria_distance)).append(": ").append(km).append('\n')
            append(getString(R.string.syria_total_time)).append(": ")
                .append(SyriaFormat.duration(this@TrackingActivity, totalSeconds(session))).append('\n')
            append(getString(R.string.syria_moving_time)).append(": ")
                .append(SyriaFormat.duration(this@TrackingActivity, session.movingTimeS)).append('\n')
            append(getString(R.string.syria_points)).append(": ").append(session.pointCount).append('\n')
            append(syncState)
        }
        val builder = MaterialAlertDialogBuilder(this)
            .setTitle(startedText(session))
            .setMessage(message)
            .setPositiveButton(android.R.string.ok, null)
        if (session.status == TrackingDb.STATUS_STOPPED) {
            builder.setNegativeButton(R.string.syria_delete) { _, _ -> confirmDelete(session) }
        }
        builder.show()
    }

    private fun confirmDelete(session: Session) {
        MaterialAlertDialogBuilder(this)
            .setMessage(R.string.syria_delete_confirm)
            .setNegativeButton(R.string.syria_cancel, null)
            .setPositiveButton(R.string.syria_delete) { _, _ ->
                Thread {
                    TrackingDb.get(this).deleteSession(session.id)
                    runOnUiThread { loadHistory() }
                }.start()
            }
            .show()
    }

    private inner class SessionAdapter : RecyclerView.Adapter<SessionHolder>() {
        private var items: List<Session> = emptyList()

        fun submit(sessions: List<Session>) {
            items = sessions
            notifyDataSetChanged()
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): SessionHolder {
            val view = LayoutInflater.from(parent.context).inflate(R.layout.item_syria_session, parent, false)
            return SessionHolder(view)
        }

        override fun getItemCount(): Int = items.size

        override fun onBindViewHolder(holder: SessionHolder, position: Int) {
            val session = items[position]
            val context = this@TrackingActivity
            holder.title.text = startedText(session)
            holder.subtitle.text = ActivityTrackingService.formatKm(context, session.distanceM) + " · " +
                getString(R.string.syria_points) + " " + session.pointCount
            holder.times.text = getString(R.string.syria_total_time) + ": " +
                SyriaFormat.duration(context, totalSeconds(session)) + "  ·  " +
                getString(R.string.syria_moving_time) + ": " + SyriaFormat.duration(context, session.movingTimeS)
            val recording = session.status != TrackingDb.STATUS_STOPPED
            holder.state.setText(
                when {
                    recording -> R.string.syria_status_active
                    session.synced -> R.string.syria_sync_status_synced
                    else -> R.string.syria_sync_status_pending
                },
            )
            val (icon, color) = when {
                recording -> R.drawable.ic_track_recording_on to R.color.syria_sync_red
                session.synced -> R.drawable.ic_syria_sync_done to R.color.syria_sync_green
                else -> R.drawable.ic_syria_sync_pending to R.color.syria_sync_orange
            }
            holder.stateIcon.setImageResource(icon)
            holder.stateIcon.imageTintList = ColorStateList.valueOf(ContextCompat.getColor(context, color))
            holder.itemView.setOnClickListener { showSession(session) }
        }
    }

    private class SessionHolder(view: View) : RecyclerView.ViewHolder(view) {
        val title: TextView = view.findViewById(R.id.session_title)
        val subtitle: TextView = view.findViewById(R.id.session_subtitle)
        val times: TextView = view.findViewById(R.id.session_times)
        val state: TextView = view.findViewById(R.id.session_state)
        val stateIcon: ImageView = view.findViewById(R.id.session_state_icon)
    }

    companion object {
        /** Opened from the driving reminder: highlight the start and live location buttons. */
        const val EXTRA_DRIVING_GUIDE = "driving_guide"
        private const val PAGE_SIZE = 10
        private const val CARD_ALPHA = 0x33
        private const val SCREEN_ALPHA = 0x1F
        private const val DISABLED_ALPHA = 0.3f
        private const val SPIN_MS = 1000L
    }
}
