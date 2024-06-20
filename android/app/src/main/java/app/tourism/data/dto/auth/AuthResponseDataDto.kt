package app.tourism.data.dto.auth

import app.tourism.domain.models.auth.AuthResponse

data class AuthResponseDto(
    val token: String,
) {
    fun toAuthResponse() = AuthResponse(
        token = token
    )
}