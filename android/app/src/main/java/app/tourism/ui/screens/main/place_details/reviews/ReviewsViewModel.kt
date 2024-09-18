package app.tourism.ui.screens.main.place_details.reviews

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.ReviewsRepository
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class ReviewsViewModel @Inject constructor(
    @ApplicationContext val context: Context,
    private val reviewsRepository: ReviewsRepository,
    private val userPreferences: UserPreferences,
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _reviews = MutableStateFlow<List<Review>>(emptyList())
    val reviews = _reviews.asStateFlow()

    private val _userReview = MutableStateFlow<Review?>(null)
    val userReview = _userReview.asStateFlow()

    fun getReviews(id: Long) {
        viewModelScope.launch(Dispatchers.IO) {
            reviewsRepository.getReviewsForPlace(id).collectLatest { resource ->
                if (resource is Resource.Success) {
                    resource.data?.let { reviewList ->
                        _reviews.value = reviewList
                        _userReview.value = reviewList.firstOrNull {
                            it.user.id == userPreferences.getUserId()?.toLong()
                        }
                    }
                }
            }
        }
        viewModelScope.launch(Dispatchers.IO) {
            reviewsRepository.getReviewsFromApi(id)
        }
    }

    fun deleteReview() {
        viewModelScope.launch(Dispatchers.IO) {
            _userReview.value?.id?.let {
                reviewsRepository.deleteReview(it).collectLatest {
                    if (it is Resource.Success) {
                        uiChannel.send(UiEvent.ShowToast(context.getString(R.string.review_deleted)))
                    }
                }
            }
        }
    }


    private val _isThereReviewPlannedToPublish = MutableStateFlow(false)
    val isThereReviewPlannedToPublish = _isThereReviewPlannedToPublish.asStateFlow()

    init {
        viewModelScope.launch(Dispatchers.IO) {
            userReview.value?.id?.let { placeId ->
                reviewsRepository.isThereReviewPlannedToPublish(placeId).collectLatest {
                    _isThereReviewPlannedToPublish.value = it
                }
            }
        }
    }
}

enum class DeleteReviewStatus { DELETED, IN_PROCESS }