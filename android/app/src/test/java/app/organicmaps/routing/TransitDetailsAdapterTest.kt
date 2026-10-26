package app.organicmaps.routing

import app.organicmaps.sdk.routing.TransitStepInfo
import app.organicmaps.sdk.routing.TransitStepType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.mockito.Mockito.doNothing
import org.mockito.Mockito.spy

/**
 * Verifies how [TransitDetailsAdapter] maps the raw transit steps onto rows: ruler segments
 * are dropped, intermediate (Add Stop) points get their own row, walks and intermediate points are not
 * rides, and only rides sharing a stop name are drawn as a joined transfer.
 *
 * The adapter is a [androidx.recyclerview.widget.RecyclerView.Adapter], so it is spied and its
 * `notifyDataSetChanged()` stubbed (matching PlaceOpeningHoursAdapterTest).
 */
class TransitDetailsAdapterTest {
    private lateinit var adapter: TransitDetailsAdapter

    @Before
    fun setUp() {
        adapter = spy(TransitDetailsAdapter())
        doNothing().`when`(adapter).notifyDataSetChanged()
    }

    @Test
    fun rulerSteps_areDropped() {
        adapter.setItems(listOf(walk(), ruler(), ride("U5")))

        assertEquals(2, adapter.itemCount)
        assertEquals(TransitDetailsAdapter.TYPE_WALK, adapter.getItemViewType(0))
        assertEquals(TransitDetailsAdapter.TYPE_RIDE, adapter.getItemViewType(1))
    }

    @Test
    fun intermediatePoint_isKeptAsItsOwnRowBetweenWalks() {
        // walk -> Add Stop -> walk -> ride: the waypoint separates its two walking legs.
        adapter.setItems(listOf(walk(), intermediate(0), walk(), ride("U5")))

        assertEquals(4, adapter.itemCount)
        assertEquals(TransitDetailsAdapter.TYPE_WALK, adapter.getItemViewType(0))
        assertEquals(TransitDetailsAdapter.TYPE_INTERMEDIATE, adapter.getItemViewType(1))
        assertEquals(TransitDetailsAdapter.TYPE_WALK, adapter.getItemViewType(2))
        assertEquals(TransitDetailsAdapter.TYPE_RIDE, adapter.getItemViewType(3))
    }

    @Test
    fun destination_isAppendedAfterTheLegs() {
        adapter.setItems(listOf(walk(), ride("U5")))
        adapter.setDestination("Hauptbahnhof")

        assertEquals(3, adapter.itemCount)
        assertEquals(TransitDetailsAdapter.TYPE_DESTINATION, adapter.getItemViewType(2))
    }

    @Test
    fun emptyDestination_addsNoRow() {
        adapter.setItems(listOf(walk(), ride("U5")))
        adapter.setDestination("")

        assertEquals(2, adapter.itemCount)
    }

    @Test
    fun isRide_excludesWalksAndIntermediatePoints() {
        adapter.setItems(listOf(walk(), intermediate(0), ride("U5")))

        assertFalse(adapter.isRide(0)) // walk
        assertFalse(adapter.isRide(1)) // intermediate point
        assertTrue(adapter.isRide(2)) // transit ride
        assertFalse(adapter.isRide(-1)) // out of bounds
        assertFalse(adapter.isRide(3)) // out of bounds
    }

    @Test
    fun sameStopTransfer_joinsRidesSharingTheStop() {
        adapter.setItems(listOf(ride("U5", "A", "B"), ride("U6", "B", "C")))

        assertFalse(adapter.isSameStopTransfer(0))
        assertTrue(adapter.isSameStopTransfer(1))
    }

    @Test
    fun transferBetweenDifferentlyNamedStops_keepsBothNames() {
        adapter.setItems(listOf(ride("U5", "A", "Incoming station"), ride("U6", "Connecting station", "C")))

        assertFalse(adapter.isSameStopTransfer(1))
    }

    @Test
    fun walkBetweenRides_isNotATransfer() {
        adapter.setItems(listOf(ride("U5", "A", "B"), walk(), ride("U6", "B", "C")))

        assertFalse(adapter.isSameStopTransfer(2))
    }

    private fun walk() = step(TransitStepType.PEDESTRIAN, null, null, null)

    private fun ride(number: String, startStop: String? = null, endStop: String? = null) =
        step(TransitStepType.SUBWAY, number, startStop, endStop)

    private fun intermediate(index: Int) = TransitStepInfo.intermediatePoint(index)

    private fun ruler() = TransitStepInfo.ruler("1", "km")

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
