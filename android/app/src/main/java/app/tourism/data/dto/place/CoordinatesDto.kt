package app.tourism.data.dto.place

import app.tourism.data.dto.PlaceLocation

data class CoordinatesDto(
    val latitude: String,
    val longitude: String
) {
    fun toPlaceLocation(name: String) =
        PlaceLocation(
            name,
            latitude.toDouble(),
            longitude.toDouble()
        )
}