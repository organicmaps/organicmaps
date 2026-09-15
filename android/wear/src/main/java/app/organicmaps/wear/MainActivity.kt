package app.organicmaps.wear

import android.app.Activity
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView
import app.organicmaps.wear.protocol.WearDistanceUnit
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataEventBuffer
import com.google.android.gms.wearable.DataItemBuffer
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.DataMapItem
import com.google.android.gms.wearable.MessageClient
import com.google.android.gms.wearable.MessageEvent
import com.google.android.gms.wearable.Node
import com.google.android.gms.wearable.PutDataRequest
import com.google.android.gms.wearable.Wearable

class MainActivity :
    Activity(),
    DataClient.OnDataChangedListener,
    MessageClient.OnMessageReceivedListener {
    private lateinit var subtitle: TextView
    private lateinit var turnDistanceValue: TextView
    private lateinit var turnDistanceUnit: TextView
    private lateinit var nextStreet: TextView
    private lateinit var remaining: TextView

    private lateinit var dataClient: DataClient
    private lateinit var messageClient: MessageClient
    private lateinit var capabilityClient: CapabilityClient

    private var currentMode = WearNavigationMode.NORMAL
    private var currentDetails: WearNavigationDetails? = null

    private var resumed = false
    private var lifecycleGeneration = 0
    private var refreshGeneration = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout =
            LinearLayout(this).apply {
                gravity = Gravity.CENTER
                orientation = LinearLayout.VERTICAL

                val padding = resources.getDimensionPixelSize(R.dimen.screen_padding)
                setPadding(padding, padding, padding, padding)
            }

        val title =
            TextView(this).apply {
                gravity = Gravity.CENTER
                setText(R.string.app_name)
                textSize = 18f
            }

        subtitle =
            TextView(this).apply {
                gravity = Gravity.CENTER
                textSize = 14f
            }

        turnDistanceValue =
            TextView(this).apply {
                gravity = Gravity.CENTER
                textSize = 28f
            }

        turnDistanceUnit =
            TextView(this).apply {
                gravity = Gravity.CENTER
                textSize = 14f
            }

        val turnDistance =
            LinearLayout(this).apply {
                gravity = Gravity.CENTER
                orientation = LinearLayout.HORIZONTAL
                addView(turnDistanceValue)
                addView(turnDistanceUnit)
            }

        nextStreet =
            TextView(this).apply {
                gravity = Gravity.CENTER
                textSize = 14f
            }

        remaining =
            TextView(this).apply {
                gravity = Gravity.CENTER
                textSize = 12f
            }

        layout.addView(title)
        layout.addView(subtitle)
        layout.addView(turnDistance)
        layout.addView(nextStreet)
        layout.addView(remaining)

        setContentView(layout)

        dataClient = Wearable.getDataClient(this)
        messageClient = Wearable.getMessageClient(this)
        capabilityClient = Wearable.getCapabilityClient(this)
    }

    override fun onResume() {
        super.onResume()

        // Listen only while visible to avoid background wakeups. Register before reading the
        // persistent item so a state change cannot be missed between the initial read and listener
        // registration.
        resumed = true
        val generation = ++lifecycleGeneration

        messageClient
            .addListener(this)
            .addOnFailureListener { exception ->
                Log.w(TAG, "Failed to listen for navigation details", exception)
            }

        dataClient
            .addListener(this, NAVIGATION_STATE_URI, DataClient.FILTER_LITERAL)
            .addOnSuccessListener {
                if (isResumed(generation)) {
                    refreshNavigationState()
                } else if (!resumed) {
                    dataClient.removeListener(this)
                }
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to listen for navigation state", exception)
                if (isResumed(generation)) {
                    refreshGeneration += 1
                    renderNormal()
                }
            }
    }

    override fun onPause() {
        resumed = false
        lifecycleGeneration += 1
        refreshGeneration += 1
        currentDetails = null

        dataClient
            .removeListener(this)
            .addOnFailureListener { exception ->
                Log.w(TAG, "Failed to stop navigation listener", exception)
            }

        messageClient
            .removeListener(this)
            .addOnFailureListener { exception ->
                Log.w(TAG, "Failed to stop navigation details listener", exception)
            }

        super.onPause()
    }

    override fun onDataChanged(dataEvents: DataEventBuffer) {
        if (resumed && NavigationStateSource.requiresRefresh(dataEvents)) {
            refreshNavigationState()
        }
    }

    override fun onMessageReceived(messageEvent: MessageEvent) {
        if (messageEvent.path != WearNavigationData.PATH_NAVIGATION_DETAILS) {
            return
        }

        val details =
            try {
                WearNavigationDetailsDataMapCodec.decode(
                    DataMap.fromByteArray(messageEvent.data),
                )
            } catch (exception: IllegalArgumentException) {
                Log.w(TAG, "Failed to decode Wear navigation details", exception)
                null
            }

        if (details == null) {
            return
        }

        runOnUiThread {
            if (resumed && currentMode == WearNavigationMode.NAVIGATION) {
                currentDetails = details
                renderNavigationDetails(details)
            }
        }
    }

    private fun refreshNavigationState() {
        val generation = ++refreshGeneration

        capabilityClient
            .getCapability(
                WearNavigationData.CAPABILITY_PHONE_APP,
                CapabilityClient.FILTER_REACHABLE,
            ).addOnSuccessListener { capability ->
                if (!isCurrent(generation)) {
                    return@addOnSuccessListener
                }

                val companionNode = NavigationStateSource.selectCompanion(capability.nodes)
                if (companionNode == null) {
                    renderNormal()
                    return@addOnSuccessListener
                }

                readNavigationState(companionNode, generation)
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to find the companion phone", exception)
                if (isCurrent(generation)) {
                    renderNormal()
                }
            }
    }

    private fun readNavigationState(companion: Node, generation: Int) {
        val uri = navigationStateUri(companion.id)

        dataClient
            .getDataItems(uri, DataClient.FILTER_LITERAL)
            .addOnSuccessListener { dataItems ->
                renderFromDataItems(dataItems, generation)
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to read navigation state", exception)
                if (isCurrent(generation)) {
                    renderNormal()
                }
            }
    }

    private fun renderFromDataItems(dataItems: DataItemBuffer, generation: Int) {
        val mode =
            try {
                // A node owns at most one DataItem at a path. Treat any unexpected result as no current
                // state.
                if (dataItems.count == 1) {
                    WearNavigationDataMapCodec.decode(
                        DataMapItem.fromDataItem(dataItems[0]).dataMap,
                    )
                } else {
                    null
                }
            } catch (exception: IllegalStateException) {
                Log.w(TAG, "Failed to decode Wear navigation state", exception)
                null
            } finally {
                dataItems.release()
            }

        if (isCurrent(generation)) {
            render(mode ?: WearNavigationMode.NORMAL)
        }
    }

    private fun isCurrent(generation: Int): Boolean = resumed && generation == refreshGeneration

    private fun isResumed(generation: Int): Boolean = resumed && generation == lifecycleGeneration

    private fun renderNormal() {
        render(WearNavigationMode.NORMAL)
    }

    private fun render(mode: WearNavigationMode) {
        currentMode = mode

        if (mode == WearNavigationMode.NORMAL) {
            currentDetails = null
            subtitle.setText(R.string.wear_no_navigation_message)
            renderNavigationDetails(null)
            return
        }

        subtitle.setText(R.string.wear_navigation_active_message)
        renderNavigationDetails(currentDetails)
    }

    private fun renderNavigationDetails(details: WearNavigationDetails?) {
        val distanceToTurn = details?.distanceToTurn

        if (distanceToTurn == null) {
            turnDistanceValue.text = ""
            turnDistanceUnit.text = ""
        } else {
            turnDistanceValue.text = distanceToTurn.value
            turnDistanceUnit.text = unitText(distanceToTurn.unit)
        }

        nextStreet.text = details?.nextStreet.orEmpty()

        val remainingParts = mutableListOf<String>()

        details?.remainingDistance?.let { distance ->
            remainingParts += "${distance.value} ${unitText(distance.unit)}"
        }

        formatRemainingTime(details?.remainingTimeSeconds)?.let { time ->
            remainingParts += time
        }

        remaining.text = remainingParts.joinToString(" | ")
    }

    private fun unitText(unit: WearDistanceUnit): String = getString(
        when (unit) {
            WearDistanceUnit.METERS -> R.string.wear_distance_unit_m
            WearDistanceUnit.KILOMETERS -> R.string.wear_distance_unit_km
            WearDistanceUnit.FEET -> R.string.wear_distance_unit_ft
            WearDistanceUnit.MILES -> R.string.wear_distance_unit_mi
        },
    )

    private fun formatRemainingTime(seconds: Int?): String? {
        if (seconds == null) {
            return null
        }

        val hours = seconds / 3600
        val minutes = (seconds % 3600) / 60

        return if (hours > 0) {
            getString(R.string.wear_time_hours_minutes, hours, minutes)
        } else {
            getString(R.string.wear_time_minutes, minutes)
        }
    }

    companion object {
        private val TAG = MainActivity::class.java.simpleName
        private val NAVIGATION_STATE_URI = navigationStateUri("*")

        private fun navigationStateUri(authority: String): Uri = Uri
            .Builder()
            .scheme(PutDataRequest.WEAR_URI_SCHEME)
            .authority(authority)
            .path(WearNavigationData.PATH_NAVIGATION_STATE)
            .build()
    }
}
