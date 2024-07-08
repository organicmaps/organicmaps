package app.tourism.domain.models.details

import java.io.File

data class ReviewToPost(
    val placeId: Long,
    val comment: String,
    val rating: Int,
    val images: List<File>,
)