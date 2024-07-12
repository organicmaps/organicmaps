package app.tourism.domain.models.profile

data class PersonalData(
    val id: Long,
    val fullName: String,
    val country: String,
    val pfpUrl: String?,
    val email: String,
    val language: String?,
    val theme: String?,
)
