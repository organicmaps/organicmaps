package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.enums.CharEnum

/**
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.avoid">Avoid</a>
 */
enum class Avoid(override val raw: Char) : CharEnum {
    Ferries('f'),
    Highways('h'),
    Tolls('t'),
    ;

    companion object {
        const val KEY: String = "avoid"
    }
}
