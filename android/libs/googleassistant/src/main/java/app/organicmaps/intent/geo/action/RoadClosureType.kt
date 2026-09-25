package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class RoadClosureType(override val raw: String) : StringEnum {
    Full("full"),
    Partial("partial"),
    ;

    companion object {
        const val KEY: String = "road_closure_type"
    }
}
