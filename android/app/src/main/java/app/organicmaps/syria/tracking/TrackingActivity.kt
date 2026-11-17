package app.organicmaps.syria.tracking

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.format.DateFormat
import android.text.format.DateUtils
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.widget.Toolbar
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.work.WorkInfo
import androidx.work.WorkManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity
import app.organicmaps.syria.SyriaPrefs
import app.organicmaps.syria.account.LoginActivity
import app.organicmaps.util.UiUtils
import app.organicmaps.util.Utils
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.util.Date

/** Start/pause/resume/stop screen with live distance and time, plus the local history. */
class TrackingActivity : BaseMwmFragmentActivity() {
    private val handler = Handler(Looper.getMainLooper())
    private var snapshot: TrackingEngine.Snapshot? = null
    private var snapshotReceivedAt = 0L

    private lateinit var status: TextView
    private lateinit var elapsed: TextView
    private lateinit var distance: TextView
    private lateinit var moving: TextView
    private lateinit var points: TextView
    private lateinit var startButton: Button
    private lateinit var pauseButton: Button
    private lateinit var stopButton: Button
    private lateinit var syncStatus: TextView
    private lateinit var historyEmpty: TextView
    private val adapter = SessionAdapter()

    private val permissionRequest =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { result ->
            if (result[Manifest.permission.ACCESS_FINE_LOCATION] == true) {
                startTracking()
            } else {
                Toast.makeText(this, R.string.location_is_disabled_long_text, Toast.LENGTH_LONG).show()
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
        if (!SyriaPrefs.isLoggedIn(this)) {
            startActivity(Intent(this, LoginActivity::class.java))
            finish()
            return
        }
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        toolbar.setTitle(R.string.syria_tracking_title)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { finish() }

        status = findViewById(R.id.tracking_status)
        elapsed = findViewById(R.id.tracking_elapsed)
        distance = findViewById(R.id.tracking_distance)
        moving = findViewById(R.id.tracking_moving)
        points = findViewById(R.id.tracking_points)
        startButton = findViewById(R.id.tracking_start)
        pauseButton = findViewById(R.id.tracking_pause)
        stopButton = findViewById(R.id.tracking_stop)
        syncStatus = findViewById(R.id.tracking_sync_status)
        historyEmpty = findViewById(R.id.tracking_history_empty)

        startButton.setOnClickListener { requestPermissionsThenStart() }
        pauseButton.setOnClickListener {
            when (snapshot?.status) {
                TrackingEngine.Status.ACTIVE -> ActivityTrackingService.pause(this)
                TrackingEngine.Status.PAUSED -> ActivityTrackingService.resume(this)
                else -> Unit
            }
        }
        stopButton.setOnClickListener { confirmStop() }
        findViewById<View>(R.id.tracking_sync_now).setOnClickListener { SyncWorker.enqueueNow(this) }

        val history = findViewById<RecyclerView>(R.id.tracking_history)
        history.layoutManager = LinearLayoutManager(this)
        history.adapter = adapter

        ActivityTrackingService.state().observe(this) {
            snapshot = it
            snapshotReceivedAt = System.currentTimeMillis()
            render()
            loadHistory()
        }
        WorkManager.getInstance(this).getWorkInfosForUniqueWorkLiveData(SyncWorker.UNIQUE_NOW).observe(this) { infos ->
            if (infos.any { it.state == WorkInfo.State.SUCCEEDED || it.state == WorkInfo.State.FAILED }) loadHistory()
        }
        ActivityTrackingService.resumeIfNeeded(this)
    }

    override fun onResume() {
        super.onResume()
        // Nothing to drive when onSafeCreate() bounced to the login screen.
        if (!::status.isInitialized) return
        handler.post(ticker)
        loadHistory()
    }

    override fun onPause() {
        handler.removeCallbacks(ticker)
        super.onPause()
    }

    private fun requestPermissionsThenStart() {
        val missing = ArrayList<String>()
        if (!granted(Manifest.permission.ACCESS_FINE_LOCATION)) {
            missing.add(Manifest.permission.ACCESS_COARSE_LOCATION)
            missing.add(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !granted(Manifest.permission.POST_NOTIFICATIONS)) {
            missing.add(Manifest.permission.POST_NOTIFICATIONS)
        }
        if (missing.isEmpty()) startTracking() else permissionRequest.launch(missing.toTypedArray())
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
        distance.text = ActivityTrackingService.formatKm(this, s?.distanceM ?: 0.0)
        points.text = (s?.pointCount ?: 0).toString()
        renderLive()
    }

    private fun renderLive() {
        val s = snapshot
        if (s == null || s.status == TrackingEngine.Status.IDLE) {
            elapsed.text = DateUtils.formatElapsedTime(0)
            moving.text = DateUtils.formatElapsedTime(0)
            return
        }
        val now = System.currentTimeMillis()
        elapsed.text = DateUtils.formatElapsedTime((now - s.startedAtMs) / 1000)
        val extra = if (s.status == TrackingEngine.Status.ACTIVE) (now - snapshotReceivedAt) / 1000 else 0L
        moving.text = DateUtils.formatElapsedTime(s.movingTimeS + extra)
    }

    private fun loadHistory() {
        Thread {
            val db = TrackingDb.get(this)
            val sessions = db.listSessions(HISTORY_LIMIT)
            val unsynced = db.unsyncedCount()
            runOnUiThread {
                if (isFinishing) return@runOnUiThread
                adapter.submit(sessions)
                historyEmpty.visibility = if (sessions.isEmpty()) View.VISIBLE else View.GONE
                syncStatus.text = if (unsynced == 0) {
                    getString(R.string.syria_synced_all)
                } else {
                    getString(R.string.syria_unsynced_format, unsynced)
                }
            }
        }.start()
    }

    private fun showSession(session: Session) {
        val started = DateFormat.getMediumDateFormat(this).format(Date(session.startedAt)) + " " +
            DateFormat.getTimeFormat(this).format(Date(session.startedAt))
        val km = ActivityTrackingService.formatKm(this, session.distanceM)
        val syncState =
            getString(if (session.synced) R.string.syria_sync_status_synced else R.string.syria_sync_status_pending)
        val message = buildString {
            append(getString(R.string.syria_distance)).append(": ").append(km).append('\n')
            append(getString(R.string.syria_moving_time)).append(": ")
                .append(DateUtils.formatElapsedTime(session.movingTimeS)).append('\n')
            append(getString(R.string.syria_points)).append(": ").append(session.pointCount).append('\n')
            append(syncState)
        }
        val builder = MaterialAlertDialogBuilder(this)
            .setTitle(started)
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
            val date = Date(session.startedAt)
            holder.title.text = DateFormat.getMediumDateFormat(holder.itemView.context).format(date) + " " +
                DateFormat.getTimeFormat(holder.itemView.context).format(date)
            holder.subtitle.text = ActivityTrackingService.formatKm(holder.itemView.context, session.distanceM) +
                " · " + DateUtils.formatElapsedTime(session.movingTimeS)
            holder.state.setText(
                when {
                    session.status != TrackingDb.STATUS_STOPPED -> R.string.syria_status_active
                    session.synced -> R.string.syria_sync_status_synced
                    else -> R.string.syria_sync_status_pending
                },
            )
            holder.itemView.setOnClickListener { showSession(session) }
        }
    }

    private class SessionHolder(view: View) : RecyclerView.ViewHolder(view) {
        val title: TextView = view.findViewById(R.id.session_title)
        val subtitle: TextView = view.findViewById(R.id.session_subtitle)
        val state: TextView = view.findViewById(R.id.session_state)
    }

    private companion object {
        const val HISTORY_LIMIT = 200
    }
}
