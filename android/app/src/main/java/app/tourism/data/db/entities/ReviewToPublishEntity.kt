package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey
import app.tourism.domain.models.details.ReviewToPost
import java.io.File
import java.net.URI

@Entity(tableName = "reviews_planned_to_post")
data class ReviewPlannedToPostEntity(
    @PrimaryKey(autoGenerate = true) val id: Long? = null,
    val placeId: Long,
    val comment: String,
    val rating: Int,
    val images: List<String>,
) {
    fun toReviewsPlannedToPostDto(): ReviewToPost {
        val imageFiles = images.map { File(it) }

        return ReviewToPost(
            placeId, comment, rating, imageFiles
        )
    }
}
