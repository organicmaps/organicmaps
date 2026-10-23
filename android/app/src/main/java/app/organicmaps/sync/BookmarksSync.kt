package app.organicmaps.sync

import android.content.Context
import android.net.ConnectivityManager
import androidx.annotation.MainThread
import androidx.core.content.ContextCompat
import androidx.core.content.edit
import androidx.core.net.ConnectivityManagerCompat
import androidx.lifecycle.ProcessLifecycleOwner
import androidx.lifecycle.lifecycleScope
import androidx.work.Constraints
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkInfo
import androidx.work.WorkManager
import app.organicmaps.MwmApplication
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.sync.BaseStateStore
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudSyncRegistry
import app.organicmaps.sdk.sync.LocalStore
import app.organicmaps.sdk.sync.PassResult
import app.organicmaps.sdk.sync.SyncEngine
import app.organicmaps.sdk.sync.SyncError
import app.organicmaps.sdk.util.Config
import app.organicmaps.sdk.util.ConnectionState
import app.organicmaps.sdk.util.log.Logger
import java.io.File
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Schedules bookmarks sync passes; does nothing without a registered provider or when sync is disabled. */
object BookmarksSync {
    private const val TAG = "BookmarksSync"
    private const val WORK_NAME = "BookmarksSync"
    private const val PREF_ENABLED = "pref_bookmarks_sync_enabled"
    private const val PREF_MOBILE_DATA = "pref_bookmarks_sync_mobile_data"
    private const val CHANGES_DELAY_SECONDS = 5L

    private var engine: SyncEngine? = null
    private var isInitialized = false
    private var isFirstLoadFinished = false
    private var hasChangesDuringPass = false
    private var isLocalApplyDeferred = false
    private var openBookmarksScreens = 0

    /** Bookmarks screens, their dialogs and search results hold ids that a reload of a category replaces. */
    val isBookmarksScreenOpen: Boolean
        get() = openBookmarksScreens > 0

    @JvmStatic
    fun isAvailable() = CloudSyncRegistry.provider != null

    /** Checks the setting first, so the provider is not created while sync is off. */
    @JvmStatic
    fun isEnabled(context: Context) = MwmApplication.prefs(context).getBoolean(PREF_ENABLED, false) && isAvailable()

    fun isMobileDataAllowed(context: Context) = MwmApplication.prefs(context).getBoolean(PREF_MOBILE_DATA, false)

    fun setMobileDataAllowed(context: Context, allowed: Boolean) {
        MwmApplication.prefs(context).edit { putBoolean(PREF_MOBILE_DATA, allowed) }
        // A waiting pass keeps the network it was enqueued with.
        enqueue(context, 0, ExistingWorkPolicy.REPLACE)
    }

    /** Starts from an empty base: an older one would turn lists deleted meanwhile into deletions in the cloud. */
    @JvmStatic
    @MainThread
    fun enable(context: Context) {
        MwmApplication.prefs(context).edit { putBoolean(PREF_ENABLED, true) }
        val engine = engine(context) ?: return
        val appContext = context.applicationContext
        // The engine lock runs this after a pass that was still running when sync was disabled, and before the next.
        ProcessLifecycleOwner.get().lifecycleScope.launch {
            engine.forgetLocalState()
            syncNow(appContext)
        }
    }

    /** Subscribes to bookmark changes once the core is initialized; the listeners do nothing while sync is off. */
    @JvmStatic
    @MainThread
    fun init(context: Context) {
        if (isInitialized) return
        isInitialized = true
        val appContext = context.applicationContext
        BookmarkManager.INSTANCE.addLoadingListener(
            object : BookmarkManager.BookmarksLoadingListener {
                override fun onBookmarksLoadingFinished() {
                    // Later loads are reloads by sync passes or imports, which report their changes.
                    if (isFirstLoadFinished) return
                    isFirstLoadFinished = true
                    enqueue(appContext, 0, ExistingWorkPolicy.KEEP)
                }
            },
        )
        BookmarkManager.INSTANCE.addBookmarksChangedListener { requestPass(appContext, CHANGES_DELAY_SECONDS) }
    }

    /**
     * Syncs the changes made during the pass that has just finished, including its own conflict copies, and
     * remembers whether it postponed remote changes.
     */
    @MainThread
    fun onPassFinished(context: Context, result: PassResult) {
        isLocalApplyDeferred = result == PassResult.Deferred
        if (!hasChangesDuringPass) return
        hasChangesDuringPass = false
        // Queued after the calling work: REPLACE would cancel it, and KEEP would drop this request while it runs.
        enqueue(context, CHANGES_DELAY_SECONDS, ExistingWorkPolicy.APPEND_OR_REPLACE)
    }

    /**
     * Called once the Place Page or a bookmarks screen is closed: applies the remote changes that a pass postponed
     * because of it right away, instead of after the retry delay.
     */
    @JvmStatic
    @MainThread
    fun onLocalApplyAllowed(context: Context) {
        if (!isLocalApplyDeferred) return
        isLocalApplyDeferred = false
        requestPass(context, 0)
    }

    @JvmStatic
    @MainThread
    fun onBookmarksScreenResumed() {
        ++openBookmarksScreens
    }

    @JvmStatic
    @MainThread
    fun onBookmarksScreenPaused(context: Context) {
        --openBookmarksScreens
        onLocalApplyAllowed(context)
    }

    @JvmStatic
    fun onForeground(context: Context) = enqueue(context, 0, ExistingWorkPolicy.KEEP)

    @JvmStatic
    fun syncNow(context: Context) = enqueue(context, 0, ExistingWorkPolicy.REPLACE)

    /** Passes use mobile data only when allowed, and roaming only if accepted. */
    @JvmStatic
    fun networkTypeFor(isMobileDataAllowed: Boolean, roamingAccepted: Boolean): NetworkType = when {
        !isMobileDataAllowed -> NetworkType.UNMETERED
        roamingAccepted -> NetworkType.CONNECTED
        else -> NetworkType.NOT_ROAMING
    }

    enum class Waiting { FOR_CONNECTION, FOR_WIFI }

    /** Why an enqueued pass can't start now, or null. */
    fun waiting(context: Context): Flow<Waiting?> {
        val appContext = context.applicationContext
        return WorkManager.getInstance(appContext).getWorkInfosForUniqueWorkFlow(WORK_NAME).map { infos ->
            when {
                infos.none { it.state == WorkInfo.State.ENQUEUED } -> null
                !ConnectionState.INSTANCE.isConnected -> Waiting.FOR_CONNECTION
                !isMobileDataAllowed(appContext) && isNetworkMetered(appContext) -> Waiting.FOR_WIFI
                else -> null
            }
        }
    }

    private fun isNetworkMetered(context: Context): Boolean {
        val connectivity = ContextCompat.getSystemService(context, ConnectivityManager::class.java) ?: return true
        return ConnectivityManagerCompat.isActiveNetworkMetered(connectivity)
    }

    @JvmStatic
    @MainThread
    fun disable(context: Context) {
        stop(context)
        CloudSyncRegistry.provider?.signOut()
    }

    /**
     * Turns sync off on this device, otherwise the next pass would upload everything again, and signs out like
     * [disable] once the data is deleted. When the deletion fails, sync is on again, so the user can retry it.
     * @return the failure, null once the data is deleted.
     */
    @MainThread
    suspend fun deleteCloudData(context: Context): SyncError? {
        val provider = CloudSyncRegistry.provider ?: return null
        val engine = engine(context) ?: return null
        stop(context)
        // Finishes even when the screen that asked for it is closed.
        return withContext(NonCancellable) {
            try {
                engine.deleteCloudData()
            } catch (e: CloudException) {
                Logger.e(TAG, "Failed to delete cloud data", e)
                MwmApplication.prefs(context).edit { putBoolean(PREF_ENABLED, true) }
                return@withContext e.error
            }
            provider.signOut()
            null
        }
    }

    /** Needs the initialized core. */
    @MainThread
    fun engine(context: Context): SyncEngine? {
        engine?.let { return it }
        val provider = CloudSyncRegistry.provider ?: return null
        val filesDir = context.filesDir
        val local = LocalStore(
            dir = File(BookmarkManager.INSTANCE.bookmarksDir),
            tmpDir = File(filesDir, "sync/tmp"),
            core = NativeCoreBookmarks(),
            main = Dispatchers.Main,
            io = Dispatchers.IO,
        )
        val baseStore = BaseStateStore(File(filesDir, "sync/${provider.id}.json"))
        return SyncEngine(provider.cloudStore(), local, baseStore, DeviceName.get(context), System::currentTimeMillis)
            .also { engine = it }
    }

    private fun stop(context: Context) {
        MwmApplication.prefs(context).edit { putBoolean(PREF_ENABLED, false) }
        WorkManager.getInstance(context).cancelUniqueWork(WORK_NAME)
    }

    private fun requestPass(context: Context, delaySeconds: Long) {
        // A running pass has read the files and REPLACE would cancel it, so the request waits for its end.
        if (engine?.isRunning == true) {
            hasChangesDuringPass = true
        } else {
            enqueue(context, delaySeconds, ExistingWorkPolicy.REPLACE)
        }
    }

    private fun enqueue(context: Context, delaySeconds: Long, policy: ExistingWorkPolicy) {
        if (!isEnabled(context)) return
        val networkType = networkTypeFor(isMobileDataAllowed(context), Config.getMobileDataRoaming())
        val constraints = Constraints.Builder().setRequiredNetworkType(networkType).build()
        val request = OneTimeWorkRequestBuilder<BookmarksSyncWork>()
            .setConstraints(constraints)
            .setInitialDelay(delaySeconds, TimeUnit.SECONDS)
            .build()
        WorkManager.getInstance(context).enqueueUniqueWork(WORK_NAME, policy, request)
    }
}
