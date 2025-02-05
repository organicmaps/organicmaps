package app.tourism.ui.screens.main.categories.categories

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.repositories.PlacesRepository
import app.tourism.domain.models.categories.PlaceCategory
import app.tourism.domain.models.common.PlaceShort
import app.tourism.domain.models.resource.Resource
import app.tourism.ui.models.SingleChoiceItem
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
class CategoriesViewModel @Inject constructor(
    @ApplicationContext val context: Context,
    private val placesRepository: PlacesRepository,
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    // region search query
    private val _query = MutableStateFlow("")
    val query = _query.asStateFlow()

    fun setQuery(value: String) {
        _query.value = value
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
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.setFavorite(item.id, isFavorite)
        }
    }

    private fun onCategoryChangeGetPlaces() {
        viewModelScope.launch {
            _selectedCategory.collectLatest { item ->
                item?.key?.let { id ->
                    val categoryId = id as Long
                    placesRepository.getPlacesByCategoryFromDbFlow(categoryId)
                        .collectLatest { resource ->
                            if (resource is Resource.Success) {
                                resource.data?.let { _places.value = it }
                            }
                        }
                }
            }
        }
    }

    private fun onCategoryChangeGetPlacesFromApiAlso() {
        viewModelScope.launch {
            _selectedCategory.collectLatest { item ->
                item?.key?.let { id ->
                    val categoryId = id as Long
                    placesRepository.getPlacesByCategoryFromApiIfThereIsChange(categoryId)
                }
            }
        }
    }

    init {
        _categories.value = listOf(
            SingleChoiceItem(PlaceCategory.Sights.id, context.getString(R.string.sights)),
            SingleChoiceItem(PlaceCategory.Restaurants.id, context.getString(R.string.restaurants)),
            SingleChoiceItem(PlaceCategory.Hotels.id, context.getString(R.string.hotels)),
        )
        _selectedCategory.value = categories.value.first()
        onCategoryChangeGetPlaces()
        onCategoryChangeGetPlacesFromApiAlso()
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}