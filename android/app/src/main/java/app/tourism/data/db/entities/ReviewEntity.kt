package app.tourism.data.db.entities

import androidx.room.Embedded
import androidx.room.Entity
import androidx.room.PrimaryKey
import app.tourism.domain.models.details.Review

@Entity(tableName = "reviews")
data class ReviewEntity(
    @PrimaryKey val id: Long,
    val placeId: Long,
    @Embedded val user: JustUser,
    val comment: String,
    val date: String,
    val rating: Int,
    val images: List<String>,
    val deletionPlanned: Boolean = false,
) {
    fun toReview() = Review(
        id = id,
        placeId = placeId,
        rating = rating,
        user = user.toUser(),
        date = date,
        comment = comment,
        picsUrls = images,
        deletionPlanned = deletionPlanned,
    )
}
