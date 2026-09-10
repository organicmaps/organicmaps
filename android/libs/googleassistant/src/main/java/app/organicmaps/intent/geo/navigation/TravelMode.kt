package app.organicmaps.intent.geo.navigation

import app.organicmaps.intent.geo.enums.CharEnum

/**
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.travel_mode">TravelMode</a>
 */
enum class TravelMode(override val raw: Char) : CharEnum {
    Bicycle('b'),
    Drive('d'),
    Taxi('x'),
    TwoWheeler('l'),
    Transit('r'),
    Walk('w'),
    ;

    companion object {
        const val KEY: String = "mode"
    }
}
