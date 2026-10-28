package app.organicmaps.sdk.wear.gms

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode

/**
 * Main-thread navigation-delivery policy, independent from Android and Play services.
 *
 * - Source freshness is independent from retransmission times.
 * - Only an active watch UI can receive packets; empty membership is cached.
 * - Late discovery results are ignored after stop/restart and after newer topology events.
 * - A discovery result always sends the latest details, never a captured old payload.
 */
internal class WearNavigationDeliveryState(
    private val nowMs: () -> Long,
    private val send: (Set<String>, WearNavigationDetails, Long) -> Unit,
) {
    private var navigationActive = false
    private var session = 0L
    private var discoverySequence = 0L
    private var pendingDiscovery: Long? = null
    private var activeNodes: Set<String> = emptySet()
    private var latestDetails: WearNavigationDetails? = null
    private var lastSourceAtMs = 0L
    private var lastSentDetails: WearNavigationDetails? = null
    private var lastSentAtMs: Long? = null

    val isActive: Boolean get() = navigationActive
    val shouldHeartbeat: Boolean get() = navigationActive && activeNodes.isNotEmpty() && freshDetails() != null

    fun setMode(mode: WearNavigationMode) {
        if (mode == WearNavigationMode.NAVIGATION) {
            if (navigationActive) return
            navigationActive = true
        } else {
            navigationActive = false
        }
        session++
        pendingDiscovery = null
        invalidateDetails()
    }

    fun publishDetails(details: WearNavigationDetails) {
        if (!navigationActive) return
        latestDetails = details
        lastSourceAtMs = nowMs()
        val lastTime = lastSentAtMs
        if (activeNodes.isEmpty()) return

        if (details != lastSentDetails ||
            lastTime == null ||
            nowMs() - lastTime >= HEARTBEAT_INTERVAL_MS
        ) {
            deliver(activeNodes, details)
        }
    }

    fun invalidateDetails() {
        latestDetails = null
        lastSentDetails = null
        lastSentAtMs = null
    }

    /** Event-delivered membership wins over any earlier in-flight discovery. */
    fun onTopologyChanged(nodeIds: Set<String>) {
        pendingDiscovery = null
        updateNodes(nodeIds)
    }

    fun beginDiscovery(): Long? {
        if (!navigationActive || pendingDiscovery != null) return null
        val token = ++discoverySequence
        pendingDiscovery = token
        return token
    }

    fun completeDiscovery(token: Long, nodeIds: Set<String>) {
        if (!navigationActive || pendingDiscovery != token) return
        pendingDiscovery = null
        updateNodes(nodeIds)
    }

    fun failDiscovery(token: Long) {
        if (pendingDiscovery == token) pendingDiscovery = null
    }

    fun heartbeat() {
        if (!navigationActive || activeNodes.isEmpty()) return
        val details = freshDetails() ?: return
        val lastTime = lastSentAtMs
        if (lastTime == null || nowMs() - lastTime >= HEARTBEAT_INTERVAL_MS) {
            deliver(activeNodes, details)
        }
    }

    fun onSendFailure(nodeId: String, sentSession: Long) {
        if (sentSession == session) {
            activeNodes = activeNodes - nodeId
        }
    }

    private fun updateNodes(nodes: Set<String>) {
        val added = nodes - activeNodes
        activeNodes = nodes
        if (navigationActive && added.isNotEmpty()) {
            freshDetails()?.let { details -> deliver(added, details) }
        }
    }

    private fun freshDetails(): WearNavigationDetails? {
        val details = latestDetails ?: return null
        val ageMs = nowMs() - lastSourceAtMs
        if (ageMs < 0L || ageMs >= SOURCE_FRESHNESS_MS) {
            invalidateDetails()
            return null
        }
        return details
    }

    private fun deliver(nodes: Set<String>, details: WearNavigationDetails) {
        if (nodes.isEmpty()) return
        lastSentDetails = details
        lastSentAtMs = nowMs()
        send(nodes, details, session)
    }

    companion object {
        const val HEARTBEAT_INTERVAL_MS = 2_000L
        private const val SOURCE_FRESHNESS_MS = 5_000L
    }
}
