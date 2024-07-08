package app.tourism.data.dto

import app.tourism.data.dto.place.PlaceDto

data class AllDataDto(
    val attractions: List<PlaceDto>,
    val restaurants: List<PlaceDto>,
    val accommodations: List<PlaceDto>,
    val attractions_hash: String,
    val restaurants_hash: String,
    val accommodations_hash: String,
)

