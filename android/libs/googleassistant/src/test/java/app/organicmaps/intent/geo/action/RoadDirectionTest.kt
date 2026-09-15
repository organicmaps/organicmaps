package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class RoadDirectionTest :
    StringEnumTestBase<RoadDirection>(
        RoadDirection.entries,
        TESTED_VALUES,
        "invalid_direction",
    ) {
    override fun fromRaw(raw: String?): RoadDirection? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            RoadDirection.ThisSide to "this_side",
            RoadDirection.OtherSide to "other_side",
        )
    }
}
