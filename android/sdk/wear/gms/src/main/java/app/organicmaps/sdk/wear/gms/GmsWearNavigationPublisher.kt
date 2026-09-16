package app.organicmaps.sdk.wear.gms

import android.content.Context
import android.os.SystemClock
import android.util.Log
import app.organicmaps.sdk.wear.WearNavigationDetailsPublisher
import app.organicmaps.sdk.wear.WearNavigationPublisher
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.common.api.ApiException
import com.google.android.gms.common.api.CommonStatusCodes
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.PutDataMapRequest
import com.google.android.gms.wearable.Wearable

/**
 * Publishes navigation data to a paired Wear OS device through the Google Wear Data Layer.
 *
 * Persistent mode uses DataClient so it can be read when the watch app starts mid-route.
 * Frequently-changing navigation details use MessageClient so they are not persisted.
 */
internal class GmsWearNavigationPublisher(context: Context) :
    WearNavigationPublisher,
    WearNavigationDetailsPublisher {
    private val context = context.applicationContext
    private val dataClient = Wearable.getDataClient(this.context)
    private val messageClient = Wearable.getMessageClient(this.context)
    private val nodeClient = Wearable.getNodeClient(this.context)

    private var connectedNodeIds: Set<String>? = null
    private var lastPublishedDetails: WearNavigationDetails? = null
    private var lastPublishedDetailsAtMs = 0L

    override fun publish(mode: WearNavigationMode) {
        if (mode == WearNavigationMode.NORMAL) {
            lastPublishedDetails = null
            lastPublishedDetailsAtMs = 0L
        }
        val dataMapRequest = PutDataMapRequest.create(WearNavigationData.PATH_NAVIGATION_STATE)
        WearNavigationDataMapCodec.encode(dataMapRequest.dataMap, mode)

        val request = dataMapRequest.asPutDataRequest()
        request.setUrgent()

        dataClient
            .putDataItem(request)
            .addOnSuccessListener {
                Log.d(TAG, "Published Wear navigation mode: $mode")
            }.addOnFailureListener { exception ->
                logFailure("navigation mode", exception)
            }
    }

    override fun publish(details: WearNavigationDetails) {
        val now = SystemClock.elapsedRealtime()
        if (details == lastPublishedDetails &&
            now - lastPublishedDetailsAtMs < DETAILS_HEARTBEAT_INTERVAL_MS
        ) {
            return
        }

        val dataMap = DataMap()
        WearNavigationDetailsDataMapCodec.encode(dataMap, details)
        val payload = dataMap.toByteArray()

        connectedNodeIds?.let { nodeIds ->
            publishDetailsToNodes(nodeIds, payload, details)
            return
        }

        nodeClient.connectedNodes
            .addOnSuccessListener { nodes ->
                val nodeIds = nodes.mapTo(mutableSetOf()) { it.id }
                if (nodeIds.isEmpty()) {
                    return@addOnSuccessListener
                }

                connectedNodeIds = nodeIds
                publishDetailsToNodes(nodeIds, payload, details)
            }.addOnFailureListener { exception ->
                connectedNodeIds = null
                logFailure("navigation details", exception)
            }
    }

    private fun publishDetailsToNodes(nodeIds: Set<String>, payload: ByteArray, details: WearNavigationDetails) {
        lastPublishedDetails = details
        lastPublishedDetailsAtMs = SystemClock.elapsedRealtime()

        nodeIds.forEach { nodeId ->
            messageClient
                .sendMessage(
                    nodeId,
                    WearNavigationData.PATH_NAVIGATION_DETAILS,
                    payload,
                ).addOnFailureListener { exception ->
                    connectedNodeIds = null
                    lastPublishedDetails = null
                    lastPublishedDetailsAtMs = 0L
                    logFailure("navigation details", exception)
                }
        }
    }

    private fun logFailure(what: String, exception: Exception) {
        if (exception is ApiException &&
            exception.statusCode == CommonStatusCodes.API_NOT_CONNECTED
        ) {
            Log.d(TAG, "Wear Data Layer unavailable, $what not published")
        } else {
            Log.w(TAG, "Failed to publish Wear $what", exception)
        }
    }

    companion object {
        private const val DETAILS_HEARTBEAT_INTERVAL_MS = 2_000L
        private val TAG = GmsWearNavigationPublisher::class.java.simpleName
    }
}
