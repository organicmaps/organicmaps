package app.tourism.data.repositories

import app.tourism.data.db.Database
import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleResponse
import app.tourism.data.remote.toFormDataRequestBody
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.details.ReviewToPost
import app.tourism.domain.models.resource.Resource
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.channelFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.flow
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.asRequestBody

class ReviewsRepository(
    private val api: TourismApi,
    private val db: Database,
) {
    private val reviewsDao = db.reviewsDao

    fun getReviewsForPlace(id: Long): Flow<Resource<List<Review>>> = channelFlow {
        reviewsDao.getReviewsForPlace(id).collectLatest { reviewsEntities ->
            val reviews = reviewsEntities.map { it.toReview() }
            send(Resource.Success(reviews))
        }
    }

    fun postReview(review: ReviewToPost): Flow<Resource<SimpleResponse>> = flow {
        val imageMultiparts = mutableListOf<MultipartBody.Part>()
        review.images.forEach {
            val requestBody = it.asRequestBody("image/*".toMediaType())
            val imageMultipart =
                MultipartBody.Part.createFormData("images[]", it.name, requestBody)
            imageMultiparts.add(imageMultipart)
        }

        emit(Resource.Loading())
        val postReviewResponse = handleResponse {
            api.postReview(
                placeId = review.placeId.toString().toFormDataRequestBody(),
                comment = review.comment.toFormDataRequestBody(),
                points = review.rating.toString().toFormDataRequestBody(),
                images = imageMultiparts
            )
        }
        emit(postReviewResponse)

        if (postReviewResponse is Resource.Success) {
            updateReviewsForDb(review.placeId)
        }
    }

    fun deleteReviews(placeId: Long, reviewsIds: List<Long>): Flow<Resource<SimpleResponse>> =
        flow {
            val deleteReviewsResponse = handleResponse {
                api.deleteReview(placeId, reviewsIds)
            }
            emit(deleteReviewsResponse)

            if (deleteReviewsResponse is Resource.Success) {
                updateReviewsForDb(placeId)
            }
        }

    private suspend fun updateReviewsForDb(id: Long) {
        val getReviewsResponse = handleResponse {
            api.getReviewsByPlaceId(id)
        }
        if (getReviewsResponse is Resource.Success) {
            reviewsDao.deleteAllPlaceReviews(id)
        }
    }
}
