package app.organicmaps.wear

import android.app.Activity
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.WearNavigationState
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataEventBuffer
import com.google.android.gms.wearable.DataItemBuffer
import com.google.android.gms.wearable.DataMapItem
import com.google.android.gms.wearable.Node
import com.google.android.gms.wearable.PutDataRequest
import com.google.android.gms.wearable.Wearable

class MainActivity :
    Activity(),
    DataClient.OnDataChangedListener {
    private lateinit var subtitle: TextView
    private lateinit var dataClient: DataClient
    private lateinit var capabilityClient: CapabilityClient

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

        layout.addView(title)
        layout.addView(subtitle)

        setContentView(layout)

        dataClient = Wearable.getDataClient(this)
        capabilityClient = Wearable.getCapabilityClient(this)
    }

    override fun onResume() {
        super.onResume()

        // Listen only while visible to avoid background wakeups. Register before reading the
        // persistent item so a state change cannot be missed between the initial read and listener
        // registration.
        resumed = true
        val generation = ++lifecycleGeneration

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

        dataClient
            .removeListener(this)
            .addOnFailureListener { exception ->
                Log.w(TAG, "Failed to stop navigation listener", exception)
            }

        super.onPause()
    }

    override fun onDataChanged(dataEvents: DataEventBuffer) {
        if (resumed && NavigationStateSource.requiresRefresh(dataEvents)) {
            refreshNavigationState()
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
        var state: WearNavigationState? = null

        try {
            // A node owns at most one DataItem at a path. Treat any unexpected result as no current
            // state.
            if (dataItems.count == 1) {
                state =
                    WearNavigationDataMapCodec.decode(
                        DataMapItem.fromDataItem(dataItems[0]).dataMap,
                    )
            }
        } catch (exception: IllegalStateException) {
            Log.w(TAG, "Failed to decode Wear navigation state", exception)
        } finally {
            dataItems.release()
        }

        if (isCurrent(generation)) {
            render(state ?: WearNavigationState.normal())
        }
    }

    private fun isCurrent(generation: Int): Boolean = resumed && generation == refreshGeneration

    private fun isResumed(generation: Int): Boolean = resumed && generation == lifecycleGeneration

    private fun renderNormal() {
        render(WearNavigationState.normal())
    }

    private fun render(state: WearNavigationState) {
        val navigating = state.mode == WearNavigationMode.NAVIGATION
        subtitle.setText(
            if (navigating) {
                R.string.wear_navigation_active_message
            } else {
                R.string.wear_no_navigation_message
            },
        )
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
