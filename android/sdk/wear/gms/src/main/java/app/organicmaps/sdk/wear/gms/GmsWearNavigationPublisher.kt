package app.organicmaps.sdk.wear.gms

import android.content.Context
import android.util.Log
import app.organicmaps.sdk.wear.WearNavigationPublisher
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationState
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import com.google.android.gms.common.api.ApiException
import com.google.android.gms.common.api.CommonStatusCodes
import com.google.android.gms.wearable.PutDataMapRequest
import com.google.android.gms.wearable.Wearable

/**
 * Publishes navigation state to a paired Wear OS device through the Google Wear Data Layer.
 *
 * Lives in the google-only `:sdk:wear:gms` module and is registered into
 * [app.organicmaps.sdk.wear.WearBridge] at startup by [WearBridgeInitProvider].
 */
internal class GmsWearNavigationPublisher(context: Context) : WearNavigationPublisher {
    private val context = context.applicationContext

    override fun publish(state: WearNavigationState) {
        val dataMapRequest = PutDataMapRequest.create(WearNavigationData.PATH_NAVIGATION_STATE)
        WearNavigationDataMapCodec.encode(dataMapRequest.dataMap, state)

        val request = dataMapRequest.asPutDataRequest()
        request.setUrgent()

        Wearable
            .getDataClient(context)
            .putDataItem(request)
            .addOnSuccessListener {
                Log.d(TAG, "Published Wear navigation state: ${state.mode}")
            }.addOnFailureListener(::logFailure)
    }

    // API_NOT_CONNECTED reports that the Wearable API is unavailable, not that delivery failed, so
    // it is expected whenever no watch is around and does not deserve a warning.
    private fun logFailure(exception: Exception) {
        if (exception is ApiException &&
            exception.statusCode == CommonStatusCodes.API_NOT_CONNECTED
        ) {
            Log.d(TAG, "Wear Data Layer unavailable, navigation state not published")
        } else {
            Log.w(TAG, "Failed to publish Wear navigation state", exception)
        }
    }

    companion object {
        private val TAG = GmsWearNavigationPublisher::class.java.simpleName
    }
}
