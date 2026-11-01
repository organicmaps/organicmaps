package app.organicmaps.sdk.wear.gms

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class WearNavigationDeliveryStateTest {
    private data class Packet(val nodes: Set<String>, val details: WearNavigationDetails)

    private var clock = 0L
    private val packets = mutableListOf<Packet>()
    private val state = WearNavigationDeliveryState({ clock }) { nodes, details ->
        packets += Packet(nodes, details)
    }

    private fun route(street: String) = WearNavigationDetails(nextStreet = street)

    @Test
    fun noTrafficWithoutAnActiveWatchIncludingWhenTopologyIsEmpty() {
        state.setMode(WearNavigationMode.NAVIGATION)
        val token = state.beginDiscovery()
        assertNotNull(token)
        assertNull(state.beginDiscovery()) // Lookup is coalesced.
        state.completeDiscovery(token!!, emptySet())
        repeat(10) {
            state.publishDetails(route("Rue Victor Hugo $it"))
            clock += 100L
            state.heartbeat()
        }
        assertFalse(state.shouldHeartbeat)
        assertTrue(packets.isEmpty())
    }

    @Test
    fun addingASecondWatchDeliversToItWithoutResendingToTheFirst() {
        state.setMode(WearNavigationMode.NAVIGATION)
        state.onTopologyChanged(setOf("watch-a"))
        state.publishDetails(route("First"))
        assertEquals(setOf("watch-a"), packets.single().nodes)
        state.onTopologyChanged(setOf("watch-a", "watch-b"))
        assertEquals(setOf("watch-b"), packets.last().nodes)
        assertEquals(2, packets.size)
        state.onTopologyChanged(emptySet())
        clock += 2_000L
        state.heartbeat()
        assertEquals(2, packets.size)
    }

    @Test
    fun staleProducerStopsHeartbeatEvenWhenTransportIsHealthy() {
        state.setMode(WearNavigationMode.NAVIGATION)
        state.onTopologyChanged(setOf("watch"))
        state.publishDetails(route("One fix"))
        clock = 2_000L
        state.heartbeat()
        clock = 4_000L
        state.heartbeat()
        assertEquals(3, packets.size)
        clock = 10_000L
        state.heartbeat()
        assertEquals(4, packets.size)
        assertTrue(state.shouldHeartbeat)
        clock = 30_000L
        state.heartbeat()
        assertEquals(4, packets.size)
        assertFalse(state.shouldHeartbeat)
        clock = 31_000L
        state.publishDetails(route("New fix"))
        assertEquals(5, packets.size)
        assertEquals("New fix", packets.last().details.nextStreet)
    }

    @Test
    fun failedInitialDiscoveryCanRetryWithoutTopologyEvent() {
        state.setMode(WearNavigationMode.NAVIGATION)
        val failed = state.beginDiscovery()!!
        assertTrue(state.failDiscovery(failed))
        val retry = state.beginDiscovery()!!
        state.publishDetails(route("New guidance"))
        state.completeDiscovery(retry, setOf("watch"))
        assertEquals("New guidance", packets.single().details.nextStreet)
        assertTrue(state.shouldHeartbeat)
    }

    @Test
    fun sourceUpdatesTenSecondsApartKeepHeartbeatActive() {
        state.setMode(WearNavigationMode.NAVIGATION)
        state.onTopologyChanged(setOf("watch"))
        state.publishDetails(route("Initial"))
        for (second in 2..20 step 2) {
            clock = second * 1_000L
            state.heartbeat()
            assertTrue(state.shouldHeartbeat)
            if (second % 10 == 0) state.publishDetails(route("Updated"))
        }
        assertTrue(packets.size >= 10)
    }

    @Test
    fun cancelledLookupDoesNotDeliverTheOldRouteAfterRestart() {
        state.setMode(WearNavigationMode.NAVIGATION)
        val old = state.beginDiscovery()!!
        state.publishDetails(route("Cancelled street"))
        state.setMode(WearNavigationMode.NORMAL)
        state.setMode(WearNavigationMode.NAVIGATION)
        val current = state.beginDiscovery()!!
        state.publishDetails(route("New street"))
        state.completeDiscovery(old, setOf("watch"))
        assertTrue(packets.isEmpty())
        state.completeDiscovery(current, setOf("watch"))
        assertEquals(1, packets.size)
        assertEquals("New street", packets.single().details.nextStreet)
    }

    @Test
    fun discoveryUsesLatestFixNotTheFixThatStartedTheLookup() {
        state.setMode(WearNavigationMode.NAVIGATION)
        val token = state.beginDiscovery()!!
        state.publishDetails(route("Earlier"))
        state.publishDetails(route("Latest"))
        state.completeDiscovery(token, setOf("watch"))
        assertEquals("Latest", packets.single().details.nextStreet)
    }

    @Test
    fun capabilityEventSupersedesAnOlderDiscoveryResult() {
        state.setMode(WearNavigationMode.NAVIGATION)
        val token = state.beginDiscovery()!!
        state.publishDetails(route("Fresh"))
        state.onTopologyChanged(setOf("watch-b"))
        state.completeDiscovery(token, setOf("watch-a"))
        assertEquals(1, packets.size)
        assertEquals(setOf("watch-b"), packets.single().nodes)
    }

    @Test
    fun invalidatedSourceAndStoppedSessionNeverRetransmit() {
        state.setMode(WearNavigationMode.NAVIGATION)
        state.onTopologyChanged(setOf("watch"))
        state.publishDetails(route("Old"))
        state.invalidateDetails()
        clock += 2_000L
        state.heartbeat()
        assertEquals(1, packets.size)
        state.setMode(WearNavigationMode.NORMAL)
        state.setMode(WearNavigationMode.NAVIGATION)
        state.publishDetails(route("New"))
        assertEquals(2, packets.size)
        assertEquals("New", packets.last().details.nextStreet)
    }
}
