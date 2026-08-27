package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class RoadDirection(override val raw: String) : StringEnum {
    ThisSide("this_side"),
    OtherSide("other_side"),
    ;

    companion object {
        const val KEY: String = "road_direction"
    }
}
