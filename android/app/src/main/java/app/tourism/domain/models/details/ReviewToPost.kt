package app.tourism.domain.models.details

import app.tourism.data.db.entities.ReviewPlannedToPostEntity
import java.io.File

data class ReviewToPost(
    val placeId: Long,
    val comment: String,
    val rating: Int,
    val images: List<File>,
) {
    fun toReviewPlannedToPostEntity(): ReviewPlannedToPostEntity {
        val imagesPaths = images.map { it.path }

        return ReviewPlannedToPostEntity(
            placeId = placeId,
            comment = comment,
            rating = rating,
            images = imagesPaths
        )
    }
}