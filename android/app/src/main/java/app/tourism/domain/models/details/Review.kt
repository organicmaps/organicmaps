package app.tourism.domain.models.details

data class Review(
    val rating: Double? = null,
    val name: String,
    val pfpUrl: String? = null,
    val countryCodeName: String? = null,
    val date: String? = null,
    val text: String? = null,
    val picsUrls: List<String> = emptyList(),
)
