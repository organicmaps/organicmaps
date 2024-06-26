package app.tourism.ui.screens.main.categories.categories

import androidx.lifecycle.ViewModel
import app.tourism.Constants
import app.tourism.domain.models.common.PlaceShort
import app.tourism.ui.models.SingleChoiceItem
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.flow.update
import javax.inject.Inject

@HiltViewModel
class CategoriesViewModel @Inject constructor(
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    // region search query
    private val _query = MutableStateFlow("")
    val query = _query.asStateFlow()

    fun setQuery(value: String) {
        _query.value = value
    }

    fun search(value: String) {
        // todo
    }

    fun clearSearchField() {
        _query.value = ""
    }
    // endregion search query


    private val _selectedCategory = MutableStateFlow<SingleChoiceItem?>(null)
    val selectedCategory = _selectedCategory.asStateFlow()

    fun setSelectedCategory(value: SingleChoiceItem?) {
        _selectedCategory.value = value
    }

    private val _categories = MutableStateFlow<List<SingleChoiceItem>>(emptyList())
    val categories = _categories.asStateFlow()


    private val _places = MutableStateFlow<List<PlaceShort>>(emptyList())
    val places = _places.asStateFlow()

    fun setFavoriteChanged(item: PlaceShort, isFavorite: Boolean) {
        // todo
    }

    init {
        // todo replace with real data
        _selectedCategory.value = SingleChoiceItem("sights", "Sights")
        _categories.value = listOf(
            SingleChoiceItem("sights", "Sights"),
            SingleChoiceItem("restaurants", "Restaurants"),
            SingleChoiceItem("hotels", "Hotels"),
        )
        val dummyData = mutableListOf<PlaceShort>()
        repeat(15) {
            dummyData.add(
                PlaceShort(
                    id = it.toLong(),
                    name = "Гора Эмина",
                    pic = Constants.IMAGE_URL_EXAMPLE,
                    rating = 5.0,
                    excerpt = "завтрак включен, бассейн, сауна, с видом на озеро"
                )
            )
        }
        _places.update { dummyData }
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}