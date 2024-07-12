package app.tourism.data.repositories

import android.content.Context
import app.tourism.data.db.Database
import app.tourism.data.dto.place.ReviewIdsDto
import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleResponse
import app.tourism.data.remote.isOnline
import app.tourism.data.remote.toFormDataRequestBody
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.details.ReviewToPost
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.channelFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.launch
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.asRequestBody
import java.io.File

class ReviewsRepository(
    @ApplicationContext val context: Context,
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

    suspend fun getReviewsFromApi(id: Long) {
        val getReviewsResponse = handleResponse { api.getReviewsByPlaceId(id) }
        if (getReviewsResponse is Resource.Success) {
            reviewsDao.deleteAllPlaceReviews(id)
            getReviewsResponse.data?.data?.map { it.toReview().toReviewEntity() }
                ?.let { reviewsDao.insertReviews(it) }
        }
    }

    fun postReview(review: ReviewToPost): Flow<Resource<SimpleResponse>> = flow {
        if (isOnline(context)) {
            emit(Resource.Loading())
            val postReviewResponse = handleResponse {
                api.postReview(
                    placeId = review.placeId.toString().toFormDataRequestBody(),
                    comment = review.comment.toFormDataRequestBody(),
                    points = review.rating.toString().toFormDataRequestBody(),
                    images = getMultipartFromImageFiles(review.images)
                )
            }
            emit(postReviewResponse)

            if (postReviewResponse is Resource.Success) {
                updateReviewsForDb(review.placeId)
            }
        } else {
            reviewsDao.insertReviewPlannedToPost(review.toReviewPlannedToPostEntity())
        }
    }

    fun deleteReview(id: Long): Flow<Resource<SimpleResponse>> =
        flow {
            reviewsDao.markReviewForDeletion(id)
            val deleteReviewResponse =
                handleResponse {
                    api.deleteReviews(ReviewIdsDto(listOf(id)))
                }

            if (deleteReviewResponse is Resource.Success) {
                reviewsDao.deleteReview(id)
            }
            emit(deleteReviewResponse)

//            val token = UserPreferences(context).getToken()
//
//            val client = OkHttpClient()
//            val mediaType = "application/json".toMediaType()
//            val body = "{\n    \"feedbacks\": [$id]\n}".toRequestBody(mediaType)
//            val request = Request.Builder()
//                .url("http://192.168.1.80:8888/api/feedbacks")
//                .method("DELETE", body)
//                .addHeader("Accept", "application/json")
//                .addHeader("Content-Type", "application/json")
//                .addHeader("Authorization", "Bearer $token")
//                .build()
//            val response = client.newCall(request).execute()

        }

    fun syncReviews() {
        val coroutineScope = CoroutineScope(Dispatchers.IO)
        coroutineScope.launch {
            deleteReviewsThatWereNotDeletedOnTheServer()
            publishReviewsThatWereNotPublished()
        }
    }

    private suspend fun deleteReviewsThatWereNotDeletedOnTheServer() {
        val reviews = reviewsDao.getReviewsPlannedForDeletion()
        if (reviews.isEmpty()) {
            val reviewsIds = reviews.map { it.id }
            val response = handleResponse { api.deleteReviews(ReviewIdsDto(reviewsIds)) }
            if (response is Resource.Success) {
                reviewsDao.deleteReviews(reviewsIds)
            }
        }
        // todo
    }

    private suspend fun publishReviewsThatWereNotPublished() {
        val reviewsPlannedToPostEntities = reviewsDao.getReviewsPlannedToPost()
        if (reviewsPlannedToPostEntities.isEmpty()) {
            val reviews = reviewsPlannedToPostEntities.map { it.toReviewsPlannedToPostDto() }
            reviews.forEach {
                CoroutineScope(Dispatchers.IO).launch {
                    api.postReview(
                        placeId = it.placeId.toString().toFormDataRequestBody(),
                        comment = it.comment.toFormDataRequestBody(),
                        points = it.rating.toString().toFormDataRequestBody(),
                        images = getMultipartFromImageFiles(it.images)
                    )
                }
            }
        }
    }

    private suspend fun updateReviewsForDb(id: Long) {
        val getReviewsResponse = handleResponse {
            api.getReviewsByPlaceId(id)
        }
        if (getReviewsResponse is Resource.Success) {
            reviewsDao.deleteAllPlaceReviews(id)
            val reviews =
                getReviewsResponse.data?.data?.map { it.toReview().toReviewEntity() } ?: listOf()
            reviewsDao.insertReviews(reviews)
        }
    }

    private fun getMultipartFromImageFiles(imageFiles: List<File>): MutableList<MultipartBody.Part> {
        val imagesMultipart = mutableListOf<MultipartBody.Part>()
        imageFiles.forEach {
            val requestBody = it.asRequestBody("image/*".toMediaType())
            val imageMultipart =
                MultipartBody.Part.createFormData("images[]", it.name, requestBody)
            imagesMultipart.add(imageMultipart)
        }
        return imagesMultipart
    }
}
