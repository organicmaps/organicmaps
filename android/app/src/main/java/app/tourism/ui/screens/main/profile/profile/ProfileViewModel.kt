package app.tourism.ui.screens.main.profile.profile

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.AuthRepository
import app.tourism.data.repositories.ProfileRepository
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.profile.PersonalData
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class ProfileViewModel @Inject constructor(
    private val profileRepository: ProfileRepository,
    private val authRepository: AuthRepository,
    private val userPreferences: UserPreferences
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _personalDataResource = MutableStateFlow<Resource<PersonalData>>(Resource.Idle())
    val profileDataResource = _personalDataResource.asStateFlow()

    fun getPersonalData() {
        viewModelScope.launch {
            profileRepository.getPersonalData()
                .collectLatest { resource ->
                    _personalDataResource.value = resource
                    if (resource is Resource.Error) {
                        uiChannel.send(UiEvent.ShowToast(resource.message ?: ""))
                    }
                }
        }
    }

    private val _signOutResponse = MutableStateFlow<Resource<SimpleResponse>>(Resource.Idle())
    val signOutResponse = _signOutResponse.asStateFlow()

    fun signOut() {
        viewModelScope.launch {
            authRepository.signOut()
                .collectLatest { resource ->
                    _signOutResponse.value = resource
                    if (resource is Resource.Success) {
                        userPreferences.setToken(null)
                        uiChannel.send(UiEvent.NavigateToAuth)
                        uiChannel.send(UiEvent.ShowToast(resource.data?.message ?: ""))
                    }
                    if (resource is Resource.Error) {
                        uiChannel.send(UiEvent.ShowToast(resource.message ?: ""))
                    }
                }
        }
    }

    init {
        getPersonalData()
    }
}

sealed interface UiEvent {
    data object NavigateToAuth : UiEvent
    data class ShowToast(val message: String) : UiEvent
}