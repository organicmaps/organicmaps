package app.tourism.ui.screens.auth.sign_up

import android.content.Context
import android.util.Patterns.EMAIL_ADDRESS
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.AuthRepository
import app.tourism.domain.models.auth.AuthResponse
import app.tourism.domain.models.auth.RegistrationData
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class SignUpViewModel @Inject constructor(
    @ApplicationContext private val context: Context,
    private val authRepository: AuthRepository,
    private val userPreferences: UserPreferences
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _registrationData =
        MutableStateFlow<RegistrationData?>(
            RegistrationData(
                "",
                "",
                "",
                "",
                "TJ"
            ),
        )
    val registrationData = _registrationData.asStateFlow()

    fun setFullName(value: String) {
        _registrationData.value = _registrationData.value?.copy(fullName = value)
    }

    fun setCountryNameCode(value: String) {
        _registrationData.value = _registrationData.value?.copy(country = value)
    }

    fun setEmail(value: String) {
        _registrationData.value = _registrationData.value?.copy(email = value)
    }

    fun setPassword(value: String) {
        _registrationData.value = _registrationData.value?.copy(password = value)
    }

    fun setConfirmPassword(value: String) {
        _registrationData.value = _registrationData.value?.copy(passwordConfirmation = value)
    }

    private val _signUpResponse = MutableStateFlow<Resource<AuthResponse>>(Resource.Idle())
    val signUpResponse = _signUpResponse.asStateFlow()

    fun signUp() {
        viewModelScope.launch {
            registrationData.value?.let {
                if (validateEverything()) {
                    authRepository.signUp(it).collectLatest { resource ->
                        _signUpResponse.value = resource
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
    }

    private fun validateEverything(): Boolean {
        return validatePasswordIsTheSame() && validateEmail()
    }

    private fun validatePasswordIsTheSame(): Boolean {
        if (registrationData.value?.password == registrationData.value?.passwordConfirmation) {
            return true
        } else {
            viewModelScope.launch {
                uiChannel.send(UiEvent.ShowToast(context.getString(R.string.passwords_not_same)))
            }
            return false
        }
    }

    private fun validateEmail(): Boolean {
        if (EMAIL_ADDRESS.matcher(registrationData.value?.email ?: "").matches())
            return true
        else {
            viewModelScope.launch {
                uiChannel.send(UiEvent.ShowToast(context.getString(R.string.wrong_email_format)))
            }
            return false
        }
    }
}

sealed interface UiEvent {
    data object NavigateToMainActivity : UiEvent
    data class ShowToast(val message: String) : UiEvent
}