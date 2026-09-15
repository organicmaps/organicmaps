package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class HazardTypeTest :
    StringEnumTestBase<HazardType>(
        HazardType.entries,
        TESTED_VALUES,
        "invalid_hazard",
    ) {
    override fun fromRaw(raw: String?): HazardType? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            HazardType.Animal to "animal",
            HazardType.BrokenTrafficLight to "broken_traffic_light",
            HazardType.Construction to "construction",
            HazardType.Flooding to "flooding",
            HazardType.Fog to "fog",
            HazardType.Hail to "hail",
            HazardType.Ice to "ice",
            HazardType.MissingSign to "missing_sign",
            HazardType.ObjectOnRoad to "object_on_road",
            HazardType.Pothole to "pothole",
            HazardType.Roadkill to "roadkill",
            HazardType.Snow to "snow",
            HazardType.Vehicle to "vehicle",
            HazardType.Weather to "weather",
        )
    }
}
