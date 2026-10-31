package app.organicmaps.sdk.wear.gms

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import app.organicmaps.sdk.wear.WearNavigationPublisher
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.common.api.ApiException
import com.google.android.gms.common.api.CommonStatusCodes
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.MessageClient
import com.google.android.gms.wearable.PutDataMapRequest
import com.google.android.gms.wearable.Wearable
import com.google.android.gms.wearable.WearableStatusCodes

/**
 * Publishes persistent navigation mode through DataClient and transient details through
 * MessageClient, targeting only watch UIs that currently advertise an active consumer.
 *
 * All mutable state, task listeners, and capability events run on the main thread.
 */
internal class GmsWearNavigationPublisher(
    context: Context,
    private val dataClient: DataClient = Wearable.getDataClient(context.applicationContext),
    private val messageClient: MessageClient = Wearable.getMessageClient(context.applicationContext),
    private val capabilityClient: CapabilityClient = Wearable.getCapabilityClient(context.applicationContext),
) : WearNavigationPublisher {
    private val handler = Handler(Looper.getMainLooper())
    private val delivery =
        WearNavigationDeliveryState(
            nowMs = SystemClock::elapsedRealtime,
            send = ::sendDetailsToNodes,
        )

    private val discoveryRetryRunnable = Runnable { discoverActiveWatches() }

    private val heartbeatRunnable =
        Runnable {
            requireMainThread()
            delivery.heartbeat()
            scheduleHeartbeat()
        }

    private val consumerListener =
        CapabilityClient.OnCapabilityChangedListener { capability ->
            requireMainThread()
            if (capability.name == WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI) {
                handler.removeCallbacks(discoveryRetryRunnable)
                delivery.onTopologyChanged(capability.nodes.mapTo(mutableSetOf()) { it.id })
                scheduleHeartbeat()
            }
        }

    init {
        // One process-lifetime listener, rather than one connectedNodes lookup per GPS fix.
        // The UI's capability is advertised dynamically and disappears on pause.
        capabilityClient
            .addListener(consumerListener, WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI)
            .addOnSuccessListener {
                requireMainThread()
                // Close the gap if a watch became active during listener registration.
                if (delivery.isActive) discoverActiveWatches()
            }.addOnFailureListener { exception ->
                logFailure("active-watch capability listener", exception)
            }
    }

    override fun publish(mode: WearNavigationMode) {
        requireMainThread()
        val newlyStarted = mode == WearNavigationMode.NAVIGATION && !delivery.isActive
        delivery.setMode(mode)
        if (mode != WearNavigationMode.NAVIGATION) handler.removeCallbacks(discoveryRetryRunnable)
        scheduleHeartbeat()
        if (newlyStarted) {
            discoverActiveWatches()
        }

        val dataMapRequest = PutDataMapRequest.create(WearNavigationData.PATH_NAVIGATION_STATE)
        WearNavigationDataMapCodec.encode(dataMapRequest.dataMap, mode)
        val request = dataMapRequest.asPutDataRequest()
        request.setUrgent()
        dataClient
            .putDataItem(request)
            .addOnSuccessListener { Log.d(TAG, "Published Wear navigation mode: $mode") }
            .addOnFailureListener { exception -> logFailure("navigation mode", exception) }
    }

    override fun publish(details: WearNavigationDetails) {
        requireMainThread()
        delivery.publishDetails(details)
        scheduleHeartbeat()
    }

    override fun invalidateDetails() {
        requireMainThread()
        // No old instruction may be retransmitted after source updates stop. The watch's own
        // expiry timer clears its last rendered instruction without another Data Layer packet.
        delivery.invalidateDetails()
        scheduleHeartbeat()
    }

    private fun discoverActiveWatches() {
        handler.removeCallbacks(discoveryRetryRunnable)
        val token = delivery.beginDiscovery() ?: return // One lookup in flight per session.
        capabilityClient
            .getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ).addOnSuccessListener { capability ->
                requireMainThread()
                // The token also rejects a late result from a previous/stopped navigation.
                delivery.completeDiscovery(token, capability.nodes.mapTo(mutableSetOf()) { it.id })
                scheduleHeartbeat()
            }.addOnFailureListener { exception ->
                requireMainThread()
                if (delivery.failDiscovery(token)) {
                    handler.postDelayed(discoveryRetryRunnable, DISCOVERY_RETRY_DELAY_MS)
                }
                logFailure("active-watch discovery", exception)
            }
    }

    private fun sendDetailsToNodes(nodeIds: Set<String>, details: WearNavigationDetails) {
        val dataMap = DataMap()
        WearNavigationDetailsDataMapCodec.encode(dataMap, details)
        val payload = dataMap.toByteArray()
        nodeIds.forEach { nodeId ->
            messageClient
                .sendMessage(nodeId, WearNavigationData.PATH_NAVIGATION_DETAILS, payload)
                .addOnFailureListener { exception ->
                    requireMainThread()
                    // Membership remains authoritative across transient send failures.
                    logFailure("navigation details", exception)
                }
        }
    }

    private fun scheduleHeartbeat() {
        handler.removeCallbacks(heartbeatRunnable)
        if (delivery.shouldHeartbeat) {
            handler.postDelayed(heartbeatRunnable, WearNavigationDeliveryState.HEARTBEAT_INTERVAL_MS)
        }
    }

    private fun requireMainThread() {
        check(Looper.myLooper() == Looper.getMainLooper()) {
            "Wear navigation publisher must be used from the main thread"
        }
    }

    private fun logFailure(what: String, exception: Exception) {
        if (exception is ApiException &&
            (
                exception.statusCode == CommonStatusCodes.API_NOT_CONNECTED ||
                    exception.statusCode == WearableStatusCodes.TARGET_NODE_NOT_CONNECTED
                )
        ) {
            Log.d(TAG, "Wear Data Layer unavailable, $what not published")
        } else {
            Log.w(TAG, "Failed to publish Wear $what", exception)
        }
    }

    companion object {
        private const val DISCOVERY_RETRY_DELAY_MS = 2_000L
        private val TAG = GmsWearNavigationPublisher::class.java.simpleName
    }
}
