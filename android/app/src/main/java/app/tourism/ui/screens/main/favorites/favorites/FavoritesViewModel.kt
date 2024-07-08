package app.tourism.ui.screens.main.favorites.favorites

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.repositories.PlacesRepository
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
class FavoritesViewModel @Inject constructor(
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

    private val _places = MutableStateFlow<List<PlaceShort>>(emptyList())
    val places = _places.asStateFlow()

    fun setFavoriteChanged(item: PlaceShort, isFavorite: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.setFavorite(item.id, isFavorite)
        }
    }

    private fun getFavorites() {
        viewModelScope.launch(Dispatchers.IO) {
            _query.collectLatest {
                placesRepository.getFavorites(it).collectLatest { resource ->
                    if (resource is Resource.Success) {
                        resource.data?.let { _places.value = it }
                    }
                }
            }
        }
    }

    init {
        getFavorites()
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}