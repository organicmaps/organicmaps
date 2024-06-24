package app.tourism.domain.models.details

import app.tourism.data.dto.PlaceLocation

data class PlaceFull(
    val id: Int,
    val name: String,
    val rating: Double? = null,
    val excerpt: String? = null,
    val description: String? = null,
    val placeLocation: PlaceLocation? = null,
    val pic: String? = null,
    val pics: List<String> = emptyList(),
    val reviews: List<Review> = emptyList(),
)
