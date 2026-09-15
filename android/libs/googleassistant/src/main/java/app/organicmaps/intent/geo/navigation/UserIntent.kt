package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.enums.StringEnum

/**
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.intent_2">User Intent</a>
 */
enum class UserIntent(override val raw: String) : StringEnum {
    Navigation("navigation"),
    AddStop("add_a_stop"),
    Directions("directions"),
    ;

    companion object {
        const val KEY: String = "intent"
    }
}
