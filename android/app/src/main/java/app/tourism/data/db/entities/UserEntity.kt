package app.tourism.data.db.entities

import app.tourism.domain.models.details.User

data class JustUser(
    val userId: Long,
    val fullName: String,
    val avatar: String?,
    val country: String
) {
    fun toUser() = User(id = userId, name = fullName, pfpUrl = avatar, countryCodeName = country)
}
