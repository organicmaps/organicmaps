package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class AccidentTypeTest :
    StringEnumTestBase<AccidentType>(
        AccidentType.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: String?): AccidentType? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            AccidentType.Minor to "minor",
            AccidentType.Major to "major",
        )
    }
}
