package app.tourism.data.dto.place

import app.tourism.data.dto.PlaceLocation

data class CoordinatesDto(
    val latitude: String?,
    val longitude: String?
) {
    fun toPlaceLocation(name: String): PlaceLocation? {
        try {
            return PlaceLocation(
                name,
                latitude!!.toDouble(),
                longitude!!.toDouble()
            )
        } catch (e: Exception) {
            e.printStackTrace()
            return null
        }
    }
}
