package app.organicmaps.wear

import android.app.Activity
import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.text.TextUtils
import android.util.Log
import android.view.Gravity
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import app.organicmaps.wear.protocol.WearDistanceUnit
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.CapabilityInfo
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

/**
 * Wear Data Layer clients used by an Activity. The provider is instance-scoped so lifecycle
 * tests can supply clients without process-global or thread-local Play services mocks.
 */
internal class WearClients(
    val dataClient: DataClient,
    val messageClient: MessageClient,
    val capabilityClient: CapabilityClient,
    val capabilityOwner: ActiveWearUiCapabilityOwner,
)

class MainActivity :
    Activity(),
    DataClient.OnDataChangedListener,
    MessageClient.OnMessageReceivedListener,
    CapabilityClient.OnCapabilityChangedListener {
    private lateinit var subtitle: TextView
    private lateinit var turnDistanceValue: TextView
    private lateinit var turnDistanceUnit: TextView
    private lateinit var nextStreet: TextView
    private lateinit var remaining: TextView

    private lateinit var dataClient: DataClient
    private lateinit var messageClient: MessageClient
    private lateinit var capabilityClient: CapabilityClient
    private lateinit var activeCapabilityOwner: ActiveWearUiCapabilityOwner

    // Defaults to real Play services; tests configure the instance before onCreate().
    internal var wearClientsFactory: (Context) -> WearClients = { context ->
        WearClients(
            dataClient = Wearable.getDataClient(context),
            messageClient = Wearable.getMessageClient(context),
            capabilityClient = Wearable.getCapabilityClient(context),
            capabilityOwner = ActiveWearUiCapabilityRegistry.get(context.applicationContext),
        )
    }

    private val uiState = WearNavigationUiState()
    private val staleDetailsRunnable = Runnable {
        if (uiState.expireDetails()) renderNavigationDetails(null)
    }
    private var refreshGeneration = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setupViews()

        val clients = wearClientsFactory(this)
        dataClient = clients.dataClient
        messageClient = clients.messageClient
        capabilityClient = clients.capabilityClient
        activeCapabilityOwner = clients.capabilityOwner
    }

    private fun setupViews() {
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
                maxLines = 2
                ellipsize = TextUtils.TruncateAt.END
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
    }

    override fun onResume() {
        super.onResume()

        // Listen only while visible to avoid background wakeups. Register before reading the
        // persistent item so a state change cannot be missed between the initial read and listener
        // registration.
        val generation = uiState.resume()

        messageClient
            .addListener(this)
            .addOnSuccessListener {
                if (isResumed(generation)) {
                    activeCapabilityOwner.activate(this)
                } else if (!uiState.resumed) {
                    messageClient.removeListener(this)
                }
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to listen for navigation details", exception)
            }

        dataClient
            .addListener(this, NAVIGATION_STATE_URI, DataClient.FILTER_LITERAL)
            .addOnSuccessListener {
                if (isResumed(generation)) {
                    refreshNavigationState()
                } else if (!uiState.resumed) {
                    dataClient.removeListener(this)
                }
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to listen for navigation state", exception)
                if (isResumed(generation)) {
                    refreshGeneration += 1
                    renderNormal()
                }
            }

        capabilityClient
            .addListener(this, WearNavigationData.CAPABILITY_PHONE_APP)
            .addOnSuccessListener {
                if (!uiState.resumed) {
                    capabilityClient.removeListener(this, WearNavigationData.CAPABILITY_PHONE_APP)
                } else {
                    refreshNavigationState()
                }
            }.addOnFailureListener { exception ->
                Log.w(TAG, "Failed to listen for phone capability changes", exception)
            }
    }

    override fun onPause() {
        uiState.pause()
        refreshGeneration += 1
        cancelStaleDetailsTimer()
        renderNavigationDetails(null)

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

        activeCapabilityOwner.deactivate(this)
        capabilityClient.removeListener(this, WearNavigationData.CAPABILITY_PHONE_APP)
            .addOnFailureListener { exception ->
                Log.w(TAG, "Failed to stop phone capability listener", exception)
            }

        super.onPause()
    }

    override fun onDataChanged(dataEvents: DataEventBuffer) {
        if (uiState.resumed && NavigationStateSource.requiresRefresh(dataEvents)) {
            refreshNavigationState()
        }
    }

    override fun onCapabilityChanged(capability: CapabilityInfo) {
        if (uiState.resumed && capability.name == WearNavigationData.CAPABILITY_PHONE_APP) {
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

        // MessageClient listeners run on the main thread by default.
        if (uiState.acceptDetails(details)) {
            renderNavigationDetails(details)
            scheduleStaleDetailsTimer()
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

    private fun isCurrent(generation: Int): Boolean = uiState.resumed && generation == refreshGeneration

    private fun isResumed(generation: Int): Boolean = uiState.isCurrent(generation)

    private fun renderNormal() {
        render(WearNavigationMode.NORMAL)
    }

    private fun render(mode: WearNavigationMode) {
        uiState.setMode(mode)

        if (mode == WearNavigationMode.NORMAL) {
            cancelStaleDetailsTimer()
            subtitle.setText(R.string.wear_no_navigation_message)
            renderNavigationDetails(null)
            return
        }

        subtitle.setText(R.string.wear_navigation_active_message)
        renderNavigationDetails(uiState.details)
    }

    private fun cancelStaleDetailsTimer() {
        remaining.removeCallbacks(staleDetailsRunnable)
    }

    private fun scheduleStaleDetailsTimer() {
        cancelStaleDetailsTimer()
        remaining.postDelayed(staleDetailsRunnable, DETAILS_STALE_TIMEOUT_MS)
    }

    private fun renderNavigationDetails(details: WearNavigationDetails?) {
        subtitle.visibility = if (details == null) View.VISIBLE else View.GONE
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
            WearDistanceUnit.METERS -> R.string.m
            WearDistanceUnit.KILOMETERS -> R.string.km
            WearDistanceUnit.FEET -> R.string.ft
            WearDistanceUnit.MILES -> R.string.mi
        },
    )

    private fun formatRemainingTime(seconds: Int?): String? = RemainingTimeFormatter.format(
        seconds,
        getString(R.string.hour),
        getString(R.string.minute),
    )

    companion object {
        private const val DETAILS_STALE_TIMEOUT_MS = 5_000L
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
