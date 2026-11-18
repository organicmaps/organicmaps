package app.organicmaps.syria

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Build
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.tracking.SyncWorker
import app.organicmaps.syria.tracking.TrackingDb
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/** Sync and internet state shown on the main map and on the recording screen. */
object SyriaStatus {
    enum class SyncState { SYNCED, PENDING, SYNCING, FAILED, SIGNED_OUT }

    /** [done] of [total] outbox items uploaded so far; both are 0 when no sync is running. */
    data class Sync(val state: SyncState, val unsynced: Int, val done: Int = 0, val total: Int = 0)

    private val executor = Executors.newSingleThreadScheduledExecutor()
    private val refreshScheduled = AtomicBoolean(false)
    private val sync = MutableLiveData<Sync>()
    private val online = MutableLiveData<Boolean>()

    @Volatile
    private var syncing = false

    @Volatile
    private var progressDone = 0

    @Volatile
    private var progressTotal = 0

    @Volatile
    private var initialized = false

    @Volatile
    private var lastOnline: Boolean? = null

    @JvmStatic
    fun sync(): LiveData<Sync> = sync

    /** From any thread; true until the network is known to be down. */
    @JvmStatic
    fun isOnline(): Boolean = lastOnline != false

    @JvmStatic
    fun online(): LiveData<Boolean> = online

    /** Starts watching the network. Called once from the application. */
    @JvmStatic
    fun init(context: Context) {
        if (initialized) return
        initialized = true
        val appContext = context.applicationContext
        val cm = appContext.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val request = NetworkRequest.Builder().addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET).build()
        cm.registerNetworkCallback(
            request,
            object : ConnectivityManager.NetworkCallback() {
                override fun onAvailable(network: Network) = updateOnline(appContext, cm)

                override fun onLost(network: Network) = updateOnline(appContext, cm)

                override fun onCapabilitiesChanged(network: Network, caps: NetworkCapabilities) =
                    updateOnline(appContext, cm)
            },
        )
        updateOnline(appContext, cm)
        refresh(appContext)
        // WorkManager cannot repeat more often than every 15 minutes (that periodic run covers a killed app);
        // while the app or a recording keeps the process alive, pending data is uploaded every minute.
        executor.scheduleWithFixedDelay(
            {
                // An exception would cancel all later runs of a scheduled task.
                try {
                    tick(appContext)
                } catch (e: RuntimeException) {
                    Logger.e(TAG, "Sync tick failed", e)
                }
            },
            SYNC_EVERY_S,
            SYNC_EVERY_S,
            TimeUnit.SECONDS,
        )
        if (SyriaPrefs.isLoggedIn(appContext)) SyncWorker.schedulePeriodic(appContext)
    }

    /** One-minute sync: only when signed in, online, idle, and something waits (or the last attempt failed). */
    private fun tick(context: Context) {
        if (!SyriaPrefs.isLoggedIn(context) || syncing || lastOnline == false) return
        val waiting = TrackingDb.get(context).unsyncedCount() > 0 || SyriaPrefs.isLastSyncFailed(context)
        // Replaces a run that is waiting out a retry backoff, so a failed sync is retried every minute too.
        if (waiting) SyncWorker.syncNowManually(context)
    }

    private fun updateOnline(context: Context, cm: ConnectivityManager) {
        val now = isOnline(cm)
        val before = lastOnline
        lastOnline = now
        if (now != before) online.postValue(now)
        // Back online: upload what was queued meanwhile instead of waiting for the worker backoff.
        if (now && before == false && SyriaPrefs.isLoggedIn(context)) SyncWorker.enqueueNow(context)
    }

    private fun isOnline(cm: ConnectivityManager): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val caps = cm.getNetworkCapabilities(cm.activeNetwork) ?: return false
            return caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) &&
                caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
        }
        @Suppress("DEPRECATION")
        return cm.activeNetworkInfo?.isConnected == true
    }

    @JvmStatic
    fun setSyncing(context: Context, value: Boolean) {
        syncing = value
        progressDone = 0
        progressTotal = 0
        refresh(context)
    }

    /** Called by the sync worker after each uploaded batch. */
    fun setProgress(context: Context, done: Int, total: Int) {
        progressTotal = maxOf(total, done)
        progressDone = done
        refresh(context)
    }

    /** Recomputes the sync state in the background. */
    @JvmStatic
    fun refresh(context: Context) {
        val appContext = context.applicationContext
        executor.execute { compute(appContext) }
    }

    /** Like [refresh], but coalesces the frequent outbox writes made while recording. */
    @JvmStatic
    fun refreshSoon(context: Context) {
        if (!refreshScheduled.compareAndSet(false, true)) return
        val appContext = context.applicationContext
        executor.schedule(
            {
                refreshScheduled.set(false)
                compute(appContext)
            },
            REFRESH_DELAY_MS,
            TimeUnit.MILLISECONDS,
        )
    }

    private fun compute(context: Context) {
        val unsynced = TrackingDb.get(context).unsyncedCount()
        val state = when {
            !SyriaPrefs.isLoggedIn(context) -> SyncState.SIGNED_OUT
            syncing -> SyncState.SYNCING
            SyriaPrefs.isLastSyncFailed(context) -> SyncState.FAILED
            unsynced > 0 -> SyncState.PENDING
            else -> SyncState.SYNCED
        }
        val value = if (state == SyncState.SYNCING) {
            Sync(state, unsynced, progressDone, progressTotal)
        } else {
            Sync(state, unsynced)
        }
        // Compared with the last posted value, not sync.value: that is only updated once the main thread
        // delivers the post, so a fast sync (SYNCING then SYNCED within a few ms) would drop the final state.
        if (lastPosted != value) {
            lastPosted = value
            sync.postValue(value)
        }
    }

    // Only touched on the executor thread.
    private var lastPosted: Sync? = null

    private const val REFRESH_DELAY_MS = 2000L
    private const val SYNC_EVERY_S = 60L
    private const val TAG = "SyriaStatus"
}
