package app.organicmaps.routing

import app.organicmaps.sdk.routing.TransitStepInfo
import app.organicmaps.sdk.routing.TransitStepType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Verifies how [TransitDetailsAdapter] maps the route steps onto rows: intermediate (Add Stop) points get
 * their own row, the destination closes the timeline, walks and intermediate points are not rides, and only
 * rides sharing a named stop are drawn as a joined transfer.
 */
class TransitDetailsAdapterTest {
    private fun adapter(steps: List<TransitStepInfo>, destination: String? = null) =
        TransitDetailsAdapter(steps, destination, emptyMap())

    @Test
    fun intermediatePoint_isKeptAsItsOwnRowBetweenWalks() {
        // walk -> Add Stop -> walk -> ride: the waypoint separates its two walking legs.
        val adapter = adapter(listOf(walk(), intermediate(0), walk(), ride("U5")))

        assertEquals(4, adapter.itemCount)
        assertEquals(TransitDetailsAdapter.TYPE_WALK, adapter.getItemViewType(0))
        assertEquals(TransitDetailsAdapter.TYPE_INTERMEDIATE, adapter.getItemViewType(1))
        assertEquals(TransitDetailsAdapter.TYPE_WALK, adapter.getItemViewType(2))
        assertEquals(TransitDetailsAdapter.TYPE_RIDE, adapter.getItemViewType(3))
    }

    @Test
    fun destination_isAppendedAfterTheLegs() {
        val adapter = adapter(listOf(walk(), ride("U5")), "Hauptbahnhof")

        assertEquals(3, adapter.itemCount)
        assertEquals(TransitDetailsAdapter.TYPE_DESTINATION, adapter.getItemViewType(2))
    }

    @Test
    fun emptyDestination_addsNoRow() {
        assertEquals(2, adapter(listOf(walk(), ride("U5")), "").itemCount)
    }

    @Test
    fun sameStopTransfer_joinsRidesSharingTheStop() {
        val adapter = adapter(listOf(ride("U5", "A", "B"), ride("U6", "B", "C")))

        assertFalse(adapter.isSameStopTransfer(0))
        assertTrue(adapter.isSameStopTransfer(1))
    }

    @Test
    fun transferBetweenDifferentlyNamedStops_keepsBothNames() {
        val adapter = adapter(listOf(ride("U5", "A", "Incoming station"), ride("U6", "Connecting station", "C")))

        assertFalse(adapter.isSameStopTransfer(1))
    }

    @Test
    fun transferBetweenUnnamedStops_isNotJoined() {
        val adapter = adapter(listOf(ride("U5", "A", ""), ride("U6", "", "C")))

        assertFalse(adapter.isSameStopTransfer(1))
    }

    @Test
    fun walkBetweenRides_isNotATransfer() {
        val adapter = adapter(listOf(ride("U5", "A", "B"), walk(), ride("U6", "B", "C")))

        assertFalse(adapter.isSameStopTransfer(2))
    }

    private fun walk() = step(TransitStepType.PEDESTRIAN, null, null, null)

    private fun ride(number: String, startStop: String? = null, endStop: String? = null) =
        step(TransitStepType.SUBWAY, number, startStop, endStop)

    private fun intermediate(index: Int) = TransitStepInfo.intermediatePoint(index)

    // TransitStepInfo's constructor is private (JNI-only); build test instances reflectively rather
    // than widening the production API.
    private fun step(type: TransitStepType, number: String?, startStop: String?, endStop: String?): TransitStepInfo {
        val ctor = TransitStepInfo::class.java.getDeclaredConstructor(
            Int::class.javaPrimitiveType, String::class.java, String::class.java, Int::class.javaPrimitiveType,
            String::class.java, Int::class.javaPrimitiveType, Int::class.javaPrimitiveType, String::class.java,
            String::class.java, Int::class.javaPrimitiveType, Array<String>::class.java, String::class.java,
        )
        ctor.isAccessible = true
        return ctor.newInstance(type.ordinal, null, null, 0, number, 0, 0, startStop, endStop, 0, null, number)
    }
}
