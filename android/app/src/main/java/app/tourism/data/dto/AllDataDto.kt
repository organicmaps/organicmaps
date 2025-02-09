package app.tourism.data.dto

import app.tourism.data.dto.place.PlaceDto

data class AllDataDto(
    val attractions_ru: List<PlaceDto>,
    val attractions_en: List<PlaceDto>,
    val restaurants_ru: List<PlaceDto>,
    val restaurants_en: List<PlaceDto>,
    val accommodations_ru: List<PlaceDto>,
    val accommodations_en: List<PlaceDto>,
    val attractions_hash: String,
    val restaurants_hash: String,
    val accommodations_hash: String,
)

