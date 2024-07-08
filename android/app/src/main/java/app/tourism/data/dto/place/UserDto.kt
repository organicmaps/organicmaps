package app.tourism.data.dto.place

import app.tourism.domain.models.details.User


data class UserDto(
    val id: Long,
    val avatar: String,
    val country: String,
    val full_name: String,
) {
    fun toUser() = User(
        id = id,
        name = full_name,
        countryCodeName = country,
        pfpUrl = avatar,
    )
}