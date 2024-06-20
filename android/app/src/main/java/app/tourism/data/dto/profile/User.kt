package app.tourism.data.dto.profile

data class User(
    val id: Int,
    val avatar: String?,
    val country_id: Any?,
    val created_at: String,
    val full_name: String,
    val language: Int,
    val phone: String,
    val theme: Int,
    val updated_at: String,
    val username: String
)