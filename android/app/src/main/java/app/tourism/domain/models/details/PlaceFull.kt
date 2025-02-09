package app.tourism.domain.models.details

import app.tourism.data.db.entities.PlaceEntity
import app.tourism.data.dto.PlaceLocation
import app.tourism.domain.models.common.PlaceShort

data class PlaceFull(
    val id: Long,
    val name: String,
    val rating: Double,
    val excerpt: String,
    val description: String,
    val placeLocation: PlaceLocation?,
    val cover: String,
    val pics: List<String> = emptyList(),
    val reviews: List<Review>? = null,
    val isFavorite: Boolean,
    val language: String
) {
    fun toPlaceShort() = PlaceShort(
        id = id,
        name = name,
        cover = cover,
        rating = rating,
        excerpt = excerpt,
        isFavorite = isFavorite
    )

    fun toPlaceEntity(categoryId: Long) = PlaceEntity(
        id = id,
        categoryId = categoryId,
        name = name,
        rating = rating,
        excerpt = excerpt,
        description = description,
        gallery = pics,
        coordinates = placeLocation?.toCoordinatesEntity(),
        cover = cover,
        isFavorite = isFavorite,
        language = language,
    )
}
