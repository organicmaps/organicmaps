package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class TrafficType(override val raw: String) : StringEnum {
    Moderate("moderate"),
    Heavy("heavy"),
    Standstill("standstill"),
    ;

    companion object {
        const val KEY: String = "traffic_type"
    }
}
