package app.tourism.domain.models.auth

data class RegistrationData(
    val fullName: String,
    val email: String,
    val password: String,
    val passwordConfirmation: String,
    val country: String,
)
