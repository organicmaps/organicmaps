package app.tourism.data.dto

import app.tourism.data.dto.place.PlaceDto

data class CategoryDto(
    val data: List<PlaceDto>,
    val hash: String
)
