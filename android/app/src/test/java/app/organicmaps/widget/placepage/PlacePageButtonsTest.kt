package app.organicmaps.widget.placepage

import app.organicmaps.sdk.routing.RoutingController
import app.organicmaps.widget.placepage.PlacePageButtons.ButtonType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.mockito.Mockito.mock
import org.mockito.Mockito.`when`

class PlacePageButtonsTest {
    private fun primaries(buttons: List<ButtonType>, needsStart: Boolean): List<ButtonType> =
        buttons.filter { PlacePageButtons.isPrimary(it, buttons, needsStart) }

    private fun trackButtons(
        defaultMax: Int,
        stopAllowed: Boolean,
        canNavigate: Boolean = true,
        prefix: List<ButtonType> = emptyList(),
    ): List<ButtonType> {
        val buttons = prefix.toMutableList()
        PlacePageButtons.addTrackButtons(buttons, defaultMax, stopAllowed, canNavigate)
        return buttons
    }

    @Test
    fun trackBarFollowsTheIosOrder() {
        assertEquals(
            listOf(
                ButtonType.ROUTE_FROM,
                ButtonType.BOOKMARK_SAVE,
                ButtonType.ROUTE_TO,
                ButtonType.TRACK_NAVIGATE,
                ButtonType.TRACK_DELETE,
            ),
            trackButtons(4, stopAllowed = false),
        )
        assertEquals(
            listOf(
                ButtonType.ROUTE_FROM,
                ButtonType.ROUTE_ADD,
                ButtonType.BOOKMARK_SAVE,
                ButtonType.TRACK_NAVIGATE,
                ButtonType.ROUTE_TO,
                ButtonType.TRACK_DELETE,
            ),
            trackButtons(4, stopAllowed = true),
        )
    }

    @Test
    fun navigateAddsOneBarSlot() {
        assertEquals(5, PlacePageButtons.getMaxButtons(trackButtons(4, stopAllowed = false), 4))
        val withoutNavigate = trackButtons(4, stopAllowed = true, canNavigate = false)
        assertEquals(
            listOf(
                ButtonType.ROUTE_FROM,
                ButtonType.ROUTE_ADD,
                ButtonType.BOOKMARK_SAVE,
                ButtonType.ROUTE_TO,
                ButtonType.TRACK_DELETE,
            ),
            withoutNavigate,
        )
        assertEquals(4, PlacePageButtons.getMaxButtons(withoutNavigate, 4))
    }

    @Test
    fun navigateNeverFallsIntoMore() {
        for (defaultMax in 2..6) {
            for (stopAllowed in listOf(false, true)) {
                for (prefix in listOf(emptyList(), listOf(ButtonType.BACK))) {
                    val buttons = trackButtons(defaultMax, stopAllowed, prefix = prefix)
                    val maxButtons = PlacePageButtons.getMaxButtons(buttons, defaultMax)
                    if (buttons.size > maxButtons) {
                        assertTrue(buttons.toString(), buttons.indexOf(ButtonType.TRACK_NAVIGATE) < maxButtons - 1)
                    }
                    assertEquals(prefix, buttons.take(prefix.size))
                }
            }
        }
    }

    @Test
    fun navigateIsTheOnlyPrimaryOnATrackPage() {
        for (stopAllowed in listOf(false, true)) {
            val buttons = trackButtons(4, stopAllowed)
            assertEquals(listOf(ButtonType.TRACK_NAVIGATE), primaries(buttons, false))
            assertEquals(listOf(ButtonType.TRACK_NAVIGATE), primaries(buttons, true))
        }
    }

    @Test
    fun followedTrackNeedsNoStart() {
        val routing = mock(RoutingController::class.java)
        assertFalse(PlacePageButtons.needsStart(routing))
        `when`(routing.isPlanning).thenReturn(true)
        assertTrue(PlacePageButtons.needsStart(routing))
        // A followed track has no route marks, so its start point is always null.
        `when`(routing.isFollowingTrack).thenReturn(true)
        assertFalse(PlacePageButtons.needsStart(routing))
        val buttons = listOf(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO)
        assertEquals(listOf(ButtonType.ROUTE_TO), primaries(buttons, PlacePageButtons.needsStart(routing)))
    }

    @Test
    fun routeToWithoutRoute() {
        val buttons = listOf(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO)
        assertEquals(listOf(ButtonType.ROUTE_TO), primaries(buttons, false))
    }

    @Test
    fun routeFromWhilePlanningWithoutStart() {
        val buttons = listOf(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO)
        assertEquals(listOf(ButtonType.ROUTE_FROM), primaries(buttons, true))
    }

    @Test
    fun addStopWhenStopIsAllowed() {
        val buttons =
            listOf(ButtonType.ROUTE_FROM, ButtonType.ROUTE_ADD, ButtonType.ROUTE_TO, ButtonType.BOOKMARK_DELETE)
        assertEquals(listOf(ButtonType.ROUTE_ADD), primaries(buttons, false))
    }

    @Test
    fun pickedPointFromRoutePlan() {
        assertEquals(
            listOf(ButtonType.ROUTE_APPEND),
            primaries(listOf(ButtonType.ROUTE_APPEND, ButtonType.BOOKMARK_SAVE), false),
        )
        assertEquals(
            listOf(ButtonType.ROUTE_REPLACE),
            primaries(listOf(ButtonType.ROUTE_REPLACE, ButtonType.BOOKMARK_SAVE), true),
        )
    }

    @Test
    fun routeToStaysPrimaryForTrackWithBackButton() {
        val buttons =
            listOf(
                ButtonType.BACK,
                ButtonType.ROUTE_FROM,
                ButtonType.BOOKMARK_SAVE,
                ButtonType.ROUTE_TO,
                ButtonType.TRACK_DELETE,
            )
        assertEquals(listOf(ButtonType.ROUTE_TO), primaries(buttons, false))
    }

    @Test
    fun nonRoutingButtonsAreNeverPrimary() {
        val routing =
            setOf(
                ButtonType.ROUTE_FROM,
                ButtonType.ROUTE_TO,
                ButtonType.ROUTE_ADD,
                ButtonType.ROUTE_APPEND,
                ButtonType.ROUTE_REPLACE,
            )
        val all = ButtonType.entries - ButtonType.TRACK_NAVIGATE
        for (type in all.filterNot { it in routing }) {
            assertFalse(type.toString(), PlacePageButtons.isPrimary(type, all, false))
            assertFalse(type.toString(), PlacePageButtons.isPrimary(type, all, true))
        }
    }
}
