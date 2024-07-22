package app.tourism.ui.screens.main.place_details

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.repositories.PlacesRepository
import app.tourism.domain.models.details.PlaceFull
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
class PlaceViewModel @Inject constructor(
    private val placesRepository: PlacesRepository
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _place = MutableStateFlow<PlaceFull?>(null)
    val place = _place.asStateFlow()

    fun setFavoriteChanged(itemId: Long, isFavorite: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.setFavorite(itemId, isFavorite)
        }
    }

     fun observePlace(id: Long) {
        viewModelScope.launch(Dispatchers.IO) {
            placesRepository.getPlaceById(id).collectLatest {
                if(it is Resource.Success) {
                    _place.value = it.data
                }
            }
        }
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}
