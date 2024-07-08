package app.tourism.ui.screens.main.home

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.repositories.PlacesRepository
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.categories.PlaceCategory
import app.tourism.domain.models.common.PlaceShort
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
class HomeViewModel @Inject constructor(
    private val placesRepository: PlacesRepository
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


    private val _sights = MutableStateFlow<List<PlaceShort>>(emptyList())
    val sights = _sights.asStateFlow()
    private fun getTopSights() {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.getTopPlaces(id = PlaceCategory.Sights.id)
                .collectLatest { resource ->
                    if (resource is Resource.Success) {
                        resource.data?.let { _sights.value = it }
                    }
                }
        }
    }


    private val _restaurants = MutableStateFlow<List<PlaceShort>>(emptyList())
    val restaurants = _restaurants.asStateFlow()
    private fun getTopRestaurants() {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.getTopPlaces(id = PlaceCategory.Restaurants.id)
                .collectLatest { resource ->
                    if (resource is Resource.Success) {
                        resource.data?.let { _restaurants.value = it }
                    }
                }
        }
    }

    private val _downloadResponse = MutableStateFlow<Resource<SimpleResponse>?>(null)
    val downloadResponse = _downloadResponse.asStateFlow()
    private fun downloadAllDataIfFirstTime() {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.downloadAllDataIfFirstTime().collectLatest {
                _downloadResponse.value = it
            }
        }
    }

    fun setFavoriteChanged(item: PlaceShort, isFavorite: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.setFavorite(item.id, isFavorite)
        }
    }

    init {
        downloadAllDataIfFirstTime()
        getTopSights()
        getTopRestaurants()
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}