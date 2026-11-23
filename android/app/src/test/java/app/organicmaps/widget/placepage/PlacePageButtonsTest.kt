package app.organicmaps.widget.placepage

import app.organicmaps.sdk.bookmarks.data.RoadWarningMarkType
import app.organicmaps.widget.placepage.PlacePageButtons.ButtonType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class PlacePageButtonsTest {
    private fun primaries(buttons: List<ButtonType>, needsStart: Boolean): List<ButtonType> =
        buttons.filter { PlacePageButtons.isPrimary(it, buttons, needsStart) }

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
                ButtonType.TRACK_DELETE,
                ButtonType.ROUTE_TO,
            )
        assertEquals(listOf(ButtonType.ROUTE_TO), primaries(buttons, false))
    }

    @Test
    fun roadWarningsOfferTheirAvoidanceAction() {
        for (warning in RoadWarningMarkType.entries) {
            val expected =
                when (warning) {
                    RoadWarningMarkType.TOLL -> ButtonType.ROUTE_AVOID_TOLL

                    RoadWarningMarkType.FERRY -> ButtonType.ROUTE_AVOID_FERRY

                    RoadWarningMarkType.DIRTY -> ButtonType.ROUTE_AVOID_UNPAVED

                    RoadWarningMarkType.STEPS -> ButtonType.ROUTE_AVOID_STEPS

                    RoadWarningMarkType.DIFFICULT_TRAIL, RoadWarningMarkType.EXPERT_TRAIL ->
                        ButtonType.ROUTE_AVOID_DIFFICULT_TRAILS

                    RoadWarningMarkType.GATE, RoadWarningMarkType.LIFT_GATE, RoadWarningMarkType.UNKNOWN -> null
                }
            assertEquals(warning.name, expected, PlacePageController.toPlacePageButton(warning))
        }
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
        val all = ButtonType.entries
        for (type in all.filterNot { it in routing }) {
            assertFalse(type.toString(), PlacePageButtons.isPrimary(type, all, false))
            assertFalse(type.toString(), PlacePageButtons.isPrimary(type, all, true))
        }
    }
}
