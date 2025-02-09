package app.tourism.ui.screens.main.place_details.reviews

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.repositories.ReviewsRepository
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.details.ReviewToPost
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.io.File
import javax.inject.Inject

@HiltViewModel
class PostReviewViewModel @Inject constructor(
    @ApplicationContext private val context: Context,
    private val reviewsRepository: ReviewsRepository
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _rating = MutableStateFlow(5f)
    val rating = _rating.asStateFlow()

    fun setRating(value: Float) {
        _rating.value = value
    }

    private val _comment = MutableStateFlow("")
    val comment = _comment.asStateFlow()

    fun setComment(value: String) {
        _comment.value = value
    }


    private val _files = MutableStateFlow<List<File>>(emptyList())
    val files = _files.asStateFlow()

    fun addFile(file: File) {
        _files.update {
            val list = _files.value.toMutableList()
            list.add(file)
            list
        }
    }

    fun removeFile(file: File) {
        _files.update {
            val list = _files.value.toMutableList()
            list.remove(file)
            list
        }
    }

    private val _postReviewResponse = MutableStateFlow<Resource<SimpleResponse>?>(null)
    val postReviewResponse = _postReviewResponse.asStateFlow()

    fun postReview(id: Long) {
        viewModelScope.launch(Dispatchers.Unconfined) {
            reviewsRepository.postReview(
                ReviewToPost(
                    placeId = id,
                    comment = _comment.value,
                    rating = _rating.value.toInt(),
                    images = _files.value
                )
            ).collectLatest {
                _postReviewResponse.value = it
                if (it is Resource.Success) {
                    uiChannel.send(
                        UiEvent.ShowToast(
                            it.message ?: context.getString(R.string.post_review_success)
                        )
                    )
                    uiChannel.send(UiEvent.CloseReviewBottomSheet)
                } else if (it is Resource.Error) {
                    uiChannel.send(
                        UiEvent.ShowToast(it.message ?: context.getString(R.string.smth_went_wrong))
                    )
                    if (it.message == context.getString(R.string.review_will_be_published_when_online)) {
                        uiChannel.send(UiEvent.CloseReviewBottomSheet)
                    }
                }
            }
        }
    }
}
