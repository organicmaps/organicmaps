package app.tourism.data.dto.profile

data class User(
    val id: Long,
    val avatar: String?,
    val country: String,
    val full_name: String,
    val language: String,
    val phone: String?,
    val theme: String,
    val username: String
)