package app.tourism.data.dto

import app.tourism.data.dto.place.PlaceDto

data class FavoritesDto(
    val ru: List<PlaceDto>,
    val en: List<PlaceDto>,
)
