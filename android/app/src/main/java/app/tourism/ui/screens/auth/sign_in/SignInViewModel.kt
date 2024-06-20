package app.tourism.ui.screens.auth.sign_in

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.AuthRepository
import app.tourism.domain.models.auth.AuthResponse
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
class SignInViewModel @Inject constructor(
    private val authRepository: AuthRepository,
    private val userPreferences: UserPreferences
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _username = MutableStateFlow("")
    val username = _username.asStateFlow()

    fun setUsername(value: String) {
        _username.value = value
    }

    private val _password = MutableStateFlow("")
    val password = _password.asStateFlow()

    fun setPassword(value: String) {
        _password.value = value
    }


    private val _signInResponse = MutableStateFlow<Resource<AuthResponse>>(Resource.Idle())
    val signInResponse = _signInResponse.asStateFlow()

    fun signIn() {
        viewModelScope.launch {
            authRepository.signIn(username.value, password.value)
                .collectLatest { resource ->
                    _signInResponse.value = resource
                    if (resource is Resource.Success) {
                        userPreferences.setToken(resource.data?.token)
                        uiChannel.send(UiEvent.NavigateToMainActivity)
                    } else if (resource is Resource.Error) {
                        uiChannel.send(UiEvent.ShowToast(resource.message ?: ""))
                    }
                }
        }
    }
}

sealed interface UiEvent {
    data object NavigateToMainActivity : UiEvent
    data class ShowToast(val message: String) : UiEvent
}