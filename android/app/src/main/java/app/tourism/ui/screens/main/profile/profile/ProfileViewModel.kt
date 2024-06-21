package app.tourism.ui.screens.main.profile.profile

import android.net.Uri
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
import java.io.File
import javax.inject.Inject

@HiltViewModel
class ProfileViewModel @Inject constructor(
    private val profileRepository: ProfileRepository,
    private val authRepository: AuthRepository,
    private val userPreferences: UserPreferences
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    // region fields to update
    private val _pfpFile = MutableStateFlow<File?>(null)
    val pfpFile = _pfpFile.asStateFlow()

    fun setPfpFile(pfpFile: File) {
        _pfpFile.value = pfpFile
    }
    
    private val _fullName = MutableStateFlow("")
    val fullName = _fullName.asStateFlow()

    fun setFullName(value: String) {
        _fullName.value = value
    }
    

    private val _email = MutableStateFlow("")
    val email = _email.asStateFlow()

    fun setEmail(value: String) {
        _email.value = value
    }


    private val _countryCodeName = MutableStateFlow<String?>(null)
    val countryCodeName = _countryCodeName.asStateFlow()

    fun setCountryCodeName(value: String) {
        _countryCodeName.value = value
    }
    // endregion fields to update

    // region requests
    private val _personalDataResource = MutableStateFlow<Resource<PersonalData>>(Resource.Idle())
    val profileDataResource = _personalDataResource.asStateFlow()

    fun getPersonalData() {
        viewModelScope.launch {
            profileRepository.getPersonalData()
                .collectLatest { resource ->
                    _personalDataResource.value = resource
                    if (resource is Resource.Success) {
                        resource.data?.let { updatePersonalDataInMemory(it) }
                    }
                    if (resource is Resource.Error) {
                        uiChannel.send(UiEvent.ShowToast(resource.message ?: ""))
                    }
                }
        }
    }


    fun save() {
        viewModelScope.launch {
            // todo
        }
    }


    private fun updatePersonalDataInMemory(personalData: PersonalData) {
        personalData.let {
            setFullName(it.fullName)
            setEmail(it.email)
            setCountryCodeName(it.country)
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
    // endregion requests

    init {
        getPersonalData()
    }
}

sealed interface UiEvent {
    data object NavigateToAuth : UiEvent
    data class ShowToast(val message: String) : UiEvent
}