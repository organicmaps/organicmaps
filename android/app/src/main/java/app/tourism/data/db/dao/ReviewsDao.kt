package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.ReviewEntity
import app.tourism.data.db.entities.ReviewPlannedToPostEntity
import kotlinx.coroutines.flow.Flow

@Dao
interface ReviewsDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReview(review: ReviewEntity)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReviews(reviews: List<ReviewEntity>)

    @Query("DELETE FROM reviews WHERE id = :id")
    suspend fun deleteReview(id: Long)

    @Query("DELETE FROM reviews WHERE id in (:idsList)")
    suspend fun deleteReviews(idsList: List<Long>)

    @Query("DELETE FROM reviews")
    suspend fun deleteAllReviews()

    @Query("DELETE FROM reviews WHERE placeId = :placeId")
    suspend fun deleteAllPlaceReviews(placeId: Long)

    @Query("SELECT * FROM reviews WHERE placeId = :placeId")
    fun getReviewsForPlace(placeId: Long): Flow<List<ReviewEntity>>

    @Query("UPDATE reviews SET deletionPlanned = :deletionPlanned WHERE id = :id")
    fun markReviewForDeletion(id: Long, deletionPlanned: Boolean = true)

    @Query("SELECT * FROM reviews WHERE deletionPlanned = 1")
    fun getReviewsPlannedForDeletion(): List<ReviewEntity>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReviewPlannedToPost(review: ReviewPlannedToPostEntity)

    @Query("DELETE FROM reviews_planned_to_post WHERE placeId = :placeId")
    suspend fun deleteReviewPlannedToPost(placeId: Long)

    @Query("SELECT * FROM reviews_planned_to_post")
    fun getReviewsPlannedToPost(): List<ReviewPlannedToPostEntity>

    @Query("SELECT * FROM reviews_planned_to_post WHERE placeId = :placeId")
    fun getReviewsPlannedToPostFlow(placeId: Long): Flow<List<ReviewPlannedToPostEntity>>
}
