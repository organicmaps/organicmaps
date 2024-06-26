package app.tourism.ui.screens.main.place_details.reviews

import androidx.lifecycle.ViewModel
import app.tourism.Constants
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.details.User
import app.tourism.utils.makeLongListOfTheSameItem
import app.tourism.utils.toUserFriendlyDate
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import javax.inject.Inject

@HiltViewModel
class ReviewsViewModel @Inject constructor(
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _reviews = MutableStateFlow<List<Review>>(emptyList())
    val reviews = _reviews.asStateFlow()

    private val _userReview = MutableStateFlow<Review?>(null)
    val userReview = _userReview.asStateFlow()

    init {
        //todo replace with real data
        _reviews.value = makeLongListOfTheSameItem(
            Review(
                id = 1,
                rating = 5.0,
                user = User(
                    id = 1,
                    name = "Эмин Уайт",
                    pfpUrl = Constants.IMAGE_URL_EXAMPLE,
                    countryCodeName = "tj",
                ),
                date = "2024-06-06".toUserFriendlyDate(),
                comment = "Это было прекрасное место! Мне очень понравилось, обязательно поситите это место gnjfhjgefkjgnjcsld\n" +
                        "Это было прекрасное место! Мне очень понравилось, обязательно поситите это место.",
                picsUrls = makeLongListOfTheSameItem(Constants.IMAGE_URL_EXAMPLE, 5)
            )
        )
    }
}
