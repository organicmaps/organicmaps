package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class LocationOnRoad(override val raw: String) : StringEnum {
    OnRoad("on_road"),
    OnShoulder("on_shoulder"),
    ;

    companion object {
        const val KEY: String = "location_on_road"
    }
}
