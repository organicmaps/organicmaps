package app.tourism.domain.models.auth

data class RegistrationData(
    val fullName: String,
    val username: String,
    val password: String,
    val passwordConfirmation: String,
    val country: String,
)
