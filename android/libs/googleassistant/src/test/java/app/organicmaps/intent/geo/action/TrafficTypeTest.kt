package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class TrafficTypeTest :
    StringEnumTestBase<TrafficType>(
        TrafficType.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: String?): TrafficType? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            TrafficType.Moderate to "moderate",
            TrafficType.Heavy to "heavy",
            TrafficType.Standstill to "standstill",
        )
    }
}
