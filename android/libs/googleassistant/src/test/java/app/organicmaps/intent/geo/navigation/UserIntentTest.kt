package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class UserIntentTest :
    StringEnumTestBase<UserIntent>(
        UserIntent.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: String?): UserIntent? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            UserIntent.Navigation to "navigation",
            UserIntent.AddStop to "add_a_stop",
            UserIntent.Directions to "directions",
        )
    }
}
