package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.CharEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class TravelModeTest :
    CharEnumTestBase<TravelMode>(
        TravelMode.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: Char): TravelMode? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            TravelMode.Bicycle to 'b',
            TravelMode.Drive to 'd',
            TravelMode.Taxi to 'x',
            TravelMode.TwoWheeler to 'l',
            TravelMode.Transit to 'r',
            TravelMode.Walk to 'w',
        )
    }
}
