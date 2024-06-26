package app.tourism.domain.models.details

data class Review(
    val id: Long,
    val rating: Double? = null,
    val user: User,
    val date: String? = null,
    val comment: String? = null,
    val picsUrls: List<String> = emptyList(),
)
