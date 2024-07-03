package app.tourism.data.dto.profile

import app.tourism.domain.models.profile.PersonalData

data class User(
    val id: Long,
    val avatar: String?,
    val country: String,
    val full_name: String,
    val email: String,
    val language: String?,
    val theme: String?,
    val username: String
) {
    fun toPersonalData() = PersonalData(
        fullName = full_name,
        country = country,
        pfpUrl = avatar,
        email = email,
        language = language,
        theme = theme,
    )
}