package app.tourism.ui.screens.main.place_details.reviews

import androidx.lifecycle.ViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.flow.update
import java.io.File
import javax.inject.Inject

@HiltViewModel
class PostReviewViewModel @Inject constructor(
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

    fun postReview() {

    }
}
