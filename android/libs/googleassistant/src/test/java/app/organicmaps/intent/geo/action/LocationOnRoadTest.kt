package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class LocationOnRoadTest :
    StringEnumTestBase<LocationOnRoad>(
        LocationOnRoad.entries,
        TESTED_VALUES,
    ) {
    override fun fromRaw(raw: String?): LocationOnRoad? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            LocationOnRoad.OnRoad to "on_road",
            LocationOnRoad.OnShoulder to "on_shoulder",
        )
    }
}
