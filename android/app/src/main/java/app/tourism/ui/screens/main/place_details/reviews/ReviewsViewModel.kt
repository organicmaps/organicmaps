package app.tourism.ui.screens.main.place_details.reviews

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.repositories.ReviewsRepository
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
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
    private val reviewsRepository: ReviewsRepository
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _reviews = MutableStateFlow<List<Review>>(emptyList())
    val reviews = _reviews.asStateFlow()

    private val _userReview = MutableStateFlow<Review?>(null)
    val userReview = _userReview.asStateFlow()

    fun getReviews(id: Long) {
        viewModelScope.launch(Dispatchers.IO) {
            reviewsRepository.getReviewsForPlace(id).collectLatest {
                if (it is Resource.Success) {
                    it.data?.let { _reviews.value = it }
                }
            }
        }
    }
}
