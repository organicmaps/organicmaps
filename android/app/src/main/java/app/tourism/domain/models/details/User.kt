package app.tourism.domain.models.details

data class User(
    val id: Long,
    val name: String,
    val pfpUrl: String? = null,
    val countryCodeName: String? = null,
)
