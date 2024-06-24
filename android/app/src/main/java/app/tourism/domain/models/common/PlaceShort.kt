package app.tourism.domain.models.common

data class PlaceShort(
    val id: Int,
    val name: String,
    val pic: String? = null,
    val rating: Double? = null,
    val excerpt: String? = null,
    val isFavorite: Boolean = false,
)
