package app.tourism.ui.screens.main.profile.profile

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.AuthRepository
import app.tourism.data.repositories.CurrencyRepository
import app.tourism.data.repositories.ProfileRepository
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.profile.CurrencyRates
import app.tourism.domain.models.profile.PersonalData
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
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
    @ApplicationContext private val context: Context,
    private val currencyRepository: CurrencyRepository,
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

    private var currentEmail = ""

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
            if (_personalDataResource.value is Resource.Success) {
                profileRepository.updateProfile(
                    fullName = fullName.value,
                    country = countryCodeName.value ?: "",
                    email = email.value,
                    pfpFile.value
                ).collectLatest { resource ->
                    if (resource is Resource.Success) {
                        resource.data?.let { updatePersonalDataInMemory(it) }
                        uiChannel.send(UiEvent.ShowToast(context.getString(R.string.saved)))
                    }
                    if (resource is Resource.Error) {
                        uiChannel.send(UiEvent.ShowToast(resource.message ?: ""))
                    }
                }
            }
        }
    }


    private fun updatePersonalDataInMemory(personalData: PersonalData) {
        personalData.let {
            _personalDataResource.value = Resource.Success(it)
            setFullName(it.fullName)
            it.email.let { email ->
                setEmail(email)
                currentEmail = email
            }
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

    // region currency
    private val _currencyRates = MutableStateFlow<CurrencyRates?>(null)
    val currencyRates = _currencyRates.asStateFlow()

    fun getCurrency() {
        viewModelScope.launch {
            currencyRepository.getCurrency().collectLatest {
                if (it is Resource.Success) {
                    _currencyRates.value = it.data
                }
            }
        }
    }
    // endregion currency

    init {
        getPersonalData()
        getCurrency()
    }
}

sealed interface UiEvent {
    data object NavigateToAuth : UiEvent
    data class ShowToast(val message: String) : UiEvent
}