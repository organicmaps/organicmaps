package app.tourism.data.db.entities

import androidx.room.Embedded
import androidx.room.Entity
import app.tourism.data.dto.PlaceLocation
import app.tourism.domain.models.common.PlaceShort
import app.tourism.domain.models.details.PlaceFull

@Entity(tableName = "places", primaryKeys = ["id", "language"] )
data class PlaceEntity(
    val id: Long,
    val categoryId: Long,
    val name: String,
    val excerpt: String,
    val description: String,
    val cover: String,
    val gallery: List<String>,
    @Embedded val coordinates: CoordinatesEntity?,
    val rating: Double,
    val isFavorite: Boolean,
    val language: String,
) {
    fun toPlaceFull() = PlaceFull(
        id = id,
        name = name,
        rating = rating,
        excerpt = excerpt,
        description = description,
        placeLocation = coordinates?.toPlaceLocation(name),
        cover = cover,
        pics = gallery,
        isFavorite = isFavorite,
        language = language
    )

    fun toPlaceShort() = PlaceShort(
        id = id,
        name = name,
        cover = cover,
        rating = rating,
        excerpt = excerpt,
        isFavorite = isFavorite
    )
}

data class CoordinatesEntity(
    val latitude: Double,
    val longitude: Double
) {
    fun toPlaceLocation(name: String) = PlaceLocation(name, latitude, longitude)
}