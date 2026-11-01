package app.organicmaps.wear

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import app.organicmaps.wear.protocol.WearNavigationData
import com.google.android.gms.wearable.Wearable

/**
 * Serializes local capability changes across Activity recreations.
 * Activity instances never remove another Activity's active consumer.
 * All transitions and Google Task callbacks are delivered on the main thread.
 */
internal class ActiveWearUiCapabilityOwner(private val changeCapability: (Boolean, (Boolean) -> Unit) -> Unit) {
    private val activeConsumers = mutableSetOf<Any>()
    private var advertised = false
    private var changePending = false

    fun activate(consumer: Any) {
        if (activeConsumers.add(consumer)) reconcile()
    }

    fun deactivate(consumer: Any) {
        if (activeConsumers.remove(consumer)) reconcile()
    }

    fun retry() {
        reconcile()
    }

    private fun reconcile() {
        val desired = activeConsumers.isNotEmpty()
        if (changePending || desired == advertised) return
        changePending = true
        changeCapability(desired) { success ->
            changePending = false
            if (success) {
                advertised = desired
                reconcile()
            }
        }
    }
}

internal object ActiveWearUiCapabilityRegistry {
    private var owner: ActiveWearUiCapabilityOwner? = null

    /** Test-only cleanup: no caller may hold an active Activity when invoked. */
    internal fun resetForTests() {
        owner = null
    }

    fun get(context: Context): ActiveWearUiCapabilityOwner {
        check(Looper.myLooper() == Looper.getMainLooper())
        return owner ?: create(context.applicationContext).also { owner = it }
    }

    private fun create(context: Context): ActiveWearUiCapabilityOwner {
        val client = Wearable.getCapabilityClient(context)
        val handler = Handler(Looper.getMainLooper())
        lateinit var sharedOwner: ActiveWearUiCapabilityOwner
        sharedOwner = ActiveWearUiCapabilityOwner { active, onComplete ->
            val task = if (active) {
                client.addLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI)
            } else {
                client.removeLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI)
            }
            task.addOnCompleteListener { result ->
                if (!result.isSuccessful) {
                    Log.w(TAG, "Failed to update active Wear UI capability", result.exception)
                    handler.postDelayed({ sharedOwner.retry() }, RETRY_DELAY_MS)
                }
                onComplete(result.isSuccessful)
            }
        }
        return sharedOwner
    }

    private const val RETRY_DELAY_MS = 2_000L
    private val TAG = ActiveWearUiCapabilityRegistry::class.java.simpleName
}
