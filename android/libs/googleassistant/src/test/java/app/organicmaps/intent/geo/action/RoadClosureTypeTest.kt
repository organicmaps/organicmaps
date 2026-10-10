package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class RoadClosureTypeTest :
    StringEnumTestBase<RoadClosureType>(
        RoadClosureType.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: String?): RoadClosureType? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            RoadClosureType.Full to "full",
            RoadClosureType.Partial to "partial",
        )
    }
}
