package app.tourism.domain.models.profile

data class PersonalData(
    val fullName: String,
    val country: String,
    val pfpUrl: String,
    val phone: String,
    val email: String
)
