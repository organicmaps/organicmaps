package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.CharEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class AvoidTest :
    CharEnumTestBase<Avoid>(
        Avoid.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: Char): Avoid? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            Avoid.Ferries to 'f',
            Avoid.Highways to 'h',
            Avoid.Tolls to 't',
        )
    }
}
