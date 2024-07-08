package app.tourism.domain.models.details

import app.tourism.data.db.entities.JustUser

data class User(
    val id: Long,
    val name: String,
    val pfpUrl: String? = null,
    val countryCodeName: String,
) {
    fun toUserEntity() = JustUser(
        userId = id, fullName = name, avatar = pfpUrl, country = countryCodeName
    )
}
