package app.tourism.domain.models.details

import app.tourism.data.db.entities.ReviewEntity

data class Review(
    val id: Long,
    val placeId: Long,
    val rating: Int,
    val user: User,
    val date: String? = null,
    val comment: String? = null,
    val picsUrls: List<String> = emptyList(),
    val deletionPlanned: Boolean = false,
) {
    fun toReviewEntity() = ReviewEntity(
        id = id,
        user = user.toUserEntity(),
        comment = comment ?: "",
        placeId = placeId,
        date = date ?: "",
        rating = rating,
        images = picsUrls,
        deletionPlanned = deletionPlanned
    )
}
