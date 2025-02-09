package app.tourism.data.dto.place

import app.tourism.domain.models.details.PlaceFull

data class PlaceDto(
    val id: Long,
    val name: String,
    val coordinates: CoordinatesDto?,
    val cover: String,
    val feedbacks: List<ReviewDto>?,
    val gallery: List<String>,
    val rating: String,
    val short_description: String,
    val long_description: String,
) {
    fun toPlaceFull(isFavorite: Boolean, language: String) = PlaceFull(
        id = id,
        name = name,
        rating = rating.toDouble(),
        excerpt = short_description,
        description = long_description,
        placeLocation = coordinates?.toPlaceLocation(name),
        cover = cover,
        pics = gallery,
        isFavorite = isFavorite,
        reviews = feedbacks?.map { it.toReview() } ?: emptyList(),
        language = language,
    )
}