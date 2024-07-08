package app.tourism.domain.models.common

data class PlaceShort(
    val id: Long,
    val name: String,
    val cover: String? = null,
    val rating: Double? = null,
    val excerpt: String? = null,
    val isFavorite: Boolean = false,
)
