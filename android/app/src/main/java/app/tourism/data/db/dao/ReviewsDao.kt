package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.ReviewEntity
import kotlinx.coroutines.flow.Flow

@Dao
interface ReviewsDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReview(review: ReviewEntity)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReviews(review: List<ReviewEntity>)

    @Delete
    suspend fun deleteReview(review: ReviewEntity)

    @Query("DELETE FROM reviews")
    suspend fun deleteAllReviews()

    @Query("DELETE FROM reviews WHERE placeId = :placeId")
    suspend fun deleteAllPlaceReviews(placeId: Long)

    @Query("SELECT * FROM reviews WHERE placeId = :placeId")
    fun getReviewsForPlace(placeId: Long): Flow<List<ReviewEntity>>
}
