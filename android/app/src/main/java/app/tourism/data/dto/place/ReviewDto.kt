package app.tourism.data.dto.place

import app.tourism.domain.models.details.Review

data class ReviewDto(
    val id: Long,
    val mark_id: Long,
    val images: List<String>,
    val message: String,
    val points: Int,
    val created_at: String,
    val user: UserDto
) {
    fun toReview() = Review(
        id = id,
        placeId = mark_id,
        rating = points,
        user = user.toUser(),
        date = created_at,
        comment = message,
        picsUrls = images
    )
}