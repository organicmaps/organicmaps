package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class HazardType(override val raw: String) : StringEnum {
    Animal("animal"),
    BrokenTrafficLight("broken_traffic_light"),
    Construction("construction"),
    Flooding("flooding"),
    Fog("fog"),
    Hail("hail"),
    Ice("ice"),
    MissingSign("missing_sign"),
    ObjectOnRoad("object_on_road"),
    Pothole("pothole"),
    Roadkill("roadkill"),
    Snow("snow"),
    Vehicle("vehicle"),
    Weather("weather"),
    ;

    companion object {
        const val KEY: String = "hazard_type"
    }
}
