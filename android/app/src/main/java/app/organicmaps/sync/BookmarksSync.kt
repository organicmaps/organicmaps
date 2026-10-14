package app.organicmaps.sync

import android.content.Context
import androidx.annotation.MainThread
import androidx.core.content.edit
import androidx.lifecycle.ProcessLifecycleOwner
import androidx.lifecycle.lifecycleScope
import androidx.work.Constraints
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkManager
import app.organicmaps.MwmApplication
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.sync.BaseStateStore
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudSyncRegistry
import app.organicmaps.sdk.sync.LocalStore
import app.organicmaps.sdk.sync.SyncEngine
import app.organicmaps.sdk.util.Config
import app.organicmaps.sdk.util.NetworkPolicy
import app.organicmaps.sdk.util.log.Logger
import java.io.File
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/** Schedules bookmarks sync passes; does nothing without a registered provider or when sync is disabled. */
object BookmarksSync {
    private const val TAG = "BookmarksSync"
    private const val WORK_NAME = "BookmarksSync"
    private const val PREF_ENABLED = "pref_bookmarks_sync_enabled"
    private const val CHANGES_DELAY_SECONDS = 5L

    private var engine: SyncEngine? = null
    private var isInitialized = false
    private var isFirstLoadFinished = false

    @JvmStatic
    fun isAvailable() = CloudSyncRegistry.provider != null

    @JvmStatic
    fun isEnabled(context: Context) = isAvailable() && MwmApplication.prefs(context).getBoolean(PREF_ENABLED, false)

    @JvmStatic
    fun setEnabled(context: Context, enabled: Boolean) {
        MwmApplication.prefs(context).edit { putBoolean(PREF_ENABLED, enabled) }
        if (enabled) syncNow(context) else WorkManager.getInstance(context).cancelUniqueWork(WORK_NAME)
    }

    /** Subscribes to bookmark changes once the core is initialized. */
    @JvmStatic
    @MainThread
    fun init(context: Context) {
        if (isInitialized || !isAvailable()) return
        isInitialized = true
        val appContext = context.applicationContext
        BookmarkManager.INSTANCE.addLoadingListener(
            object : BookmarkManager.BookmarksLoadingListener {
                override fun onBookmarksLoadingFinished() {
                    // Later loads are reloads that sync passes run themselves.
                    if (isFirstLoadFinished) return
                    isFirstLoadFinished = true
                    enqueue(appContext, 0, ExistingWorkPolicy.KEEP)
                }
            },
        )
        BookmarkManager.INSTANCE.addBookmarksChangedListener {
            // Changes during a pass are the pass's own reloads.
            if (engine?.isRunning != true) enqueue(appContext, CHANGES_DELAY_SECONDS, ExistingWorkPolicy.REPLACE)
        }
    }

    @JvmStatic
    fun onForeground(context: Context) = enqueue(context, 0, ExistingWorkPolicy.KEEP)

    @JvmStatic
    fun onBackground(context: Context) = enqueue(context, 0, ExistingWorkPolicy.KEEP)

    @JvmStatic
    fun syncNow(context: Context) = enqueue(context, 0, ExistingWorkPolicy.REPLACE)

    /** Background passes can't ask about mobile data, so only "Always" allows it, and roaming only if accepted. */
    @JvmStatic
    fun networkTypeFor(policy: NetworkPolicy.Type, roamingAccepted: Boolean): NetworkType = when {
        policy != NetworkPolicy.Type.ALWAYS -> NetworkType.UNMETERED
        roamingAccepted -> NetworkType.CONNECTED
        else -> NetworkType.NOT_ROAMING
    }

    /** Forgets the sync state before signing out, so enabling sync again never propagates deletions. */
    @JvmStatic
    @MainThread
    fun disable(context: Context) {
        setEnabled(context, false)
        engine(context)?.forgetLocalState()
        val provider = CloudSyncRegistry.provider ?: return
        ProcessLifecycleOwner.get().lifecycleScope.launch { provider.signOut() }
    }

    /** Turns sync off on this device, otherwise the next pass would upload everything again. */
    @JvmStatic
    @MainThread
    fun deleteCloudData(context: Context) {
        setEnabled(context, false)
        val engine = engine(context) ?: return
        ProcessLifecycleOwner.get().lifecycleScope.launch {
            try {
                engine.deleteCloudData()
            } catch (e: CloudException) {
                Logger.e(TAG, "Failed to delete cloud data", e)
            }
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

    private fun enqueue(context: Context, delaySeconds: Long, policy: ExistingWorkPolicy) {
        if (!isEnabled(context)) return
        val constraints = Constraints.Builder()
            .setRequiredNetworkType(networkTypeFor(Config.getUseMobileDataSettings(), Config.getMobileDataRoaming()))
            .build()
        val request = OneTimeWorkRequestBuilder<BookmarksSyncWork>()
            .setConstraints(constraints)
            .setInitialDelay(delaySeconds, TimeUnit.SECONDS)
            .build()
        WorkManager.getInstance(context).enqueueUniqueWork(WORK_NAME, policy, request)
    }
}
