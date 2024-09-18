package app.tourism.data.repositories

import android.content.Context
import app.organicmaps.R
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
import app.tourism.utils.compress
import app.tourism.utils.saveToInternalStorage
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

    fun isThereReviewPlannedToPublish(placeId: Long): Flow<Boolean> = channelFlow {
        reviewsDao.getReviewsPlannedToPostFlow(placeId).collectLatest { reviewsEntities ->
            send(reviewsEntities.isNotEmpty())
        }
    }

    suspend fun getReviewsFromApi(id: Long) {
        val getReviewsResponse = handleResponse(call = { api.getReviewsByPlaceId(id) }, context)
        if (getReviewsResponse is Resource.Success) {
            reviewsDao.deleteAllPlaceReviews(id)
            getReviewsResponse.data?.data?.map { it.toReview().toReviewEntity() }
                ?.let { reviewsDao.insertReviews(it) }
        }
    }

    fun postReview(review: ReviewToPost): Flow<Resource<SimpleResponse>> = flow {
        emit(Resource.Loading())
        val imageFiles = mutableListOf<File>()
        review.images.forEach { imageFiles.add(compress(it, context)) }
        
        if (isOnline(context)) {
            val postReviewResponse = handleResponse(
                call = {
                    api.postReview(
                        placeId = review.placeId.toString().toFormDataRequestBody(),
                        comment = review.comment.toFormDataRequestBody(),
                        points = review.rating.toString().toFormDataRequestBody(),
                        images = getMultipartFromImageFiles(imageFiles)
                    )
                },
                context
            )

            if (postReviewResponse is Resource.Success) {
                updateReviewsForDb(review.placeId)
                emit(Resource.Success(SimpleResponse(context.getString(R.string.review_was_published))))
            } else if (postReviewResponse is Resource.Error) {
                emit(Resource.Error(postReviewResponse.message ?: ""))
            }
        } else {
            try {
                saveToInternalStorage(imageFiles, context)
                reviewsDao.insertReviewPlannedToPost(review.toReviewPlannedToPostEntity(imageFiles))
                emit(Resource.Error(context.getString(R.string.review_will_be_published_when_online)))
            } catch (e: OutOfMemoryError) {
                e.printStackTrace()
                emit(Resource.Error(context.getString(R.string.smth_went_wrong)))
            }
        }
    }

    fun deleteReview(id: Long): Flow<Resource<SimpleResponse>> =
        flow {
            if (isOnline(context)) {
                val deleteReviewResponse =
                    handleResponse(
                        call = { api.deleteReviews(ReviewIdsDto(listOf(id))) },
                        context,
                    )

                if (deleteReviewResponse is Resource.Success) {
                    reviewsDao.deleteReview(id)
                }
                emit(deleteReviewResponse)
            } else {
                reviewsDao.markReviewForDeletion(id)
            }
        }

    suspend fun syncReviews() {
        deleteReviewsThatWereNotDeletedOnTheServer()
        publishReviewsThatWereNotPublished()
    }

    private suspend fun deleteReviewsThatWereNotDeletedOnTheServer() {
        val reviews = reviewsDao.getReviewsPlannedForDeletion()
        if (reviews.isNotEmpty()) {
            val reviewsIds = reviews.map { it.id }
            val response =
                handleResponse(call = { api.deleteReviews(ReviewIdsDto(reviewsIds)) }, context)
            if (response is Resource.Success) {
                reviewsDao.deleteReviews(reviewsIds)
            }
        }
    }

    private suspend fun publishReviewsThatWereNotPublished() {
        val reviewsPlannedToPostEntities = reviewsDao.getReviewsPlannedToPost()
        if (reviewsPlannedToPostEntities.isNotEmpty()) {
            val reviews = reviewsPlannedToPostEntities.map { it.toReviewsPlannedToPostDto() }
            reviews.forEach {
                CoroutineScope(Dispatchers.IO).launch {
                    val response = handleResponse(
                        call = {
                            api.postReview(
                                placeId = it.placeId.toString().toFormDataRequestBody(),
                                comment = it.comment.toFormDataRequestBody(),
                                points = it.rating.toString().toFormDataRequestBody(),
                                images = getMultipartFromImageFiles(it.images)
                            )
                        },
                        context,
                    )
                    if (response is Resource.Success) {
                        try {
                            updateReviewsForDb(it.placeId)
                            reviewsDao.deleteReviewPlannedToPost(it.placeId)
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    } else if (response is Resource.Error) {
                        try {
                            reviewsDao.deleteReviewPlannedToPost(it.placeId)
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                }
            }
        }
    }

    private suspend fun updateReviewsForDb(id: Long) {
        val getReviewsResponse = handleResponse(
            call = { api.getReviewsByPlaceId(id) },
            context
        )
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
