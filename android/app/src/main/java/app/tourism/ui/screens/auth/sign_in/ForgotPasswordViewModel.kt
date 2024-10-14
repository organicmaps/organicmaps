package app.tourism.ui.screens.auth.sign_in

import android.content.Context
import androidx.compose.ui.res.stringResource
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import app.organicmaps.R
import app.tourism.data.repositories.AuthRepository
import app.tourism.domain.models.SimpleResponse
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
class ForgotPasswordViewModel @Inject constructor(
    @ApplicationContext val context: Context,
    private val authRepository: AuthRepository,
) : ViewModel() {
    private val uiChannel = Channel<ForgotPasswordUiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _email = MutableStateFlow("")
    val email = _email.asStateFlow()

    fun setEmail(value: String) {
        _email.value = value
    }

    private val _forgotPasswordResponse =
        MutableStateFlow<Resource<SimpleResponse>>(Resource.Idle())
    val forgotPasswordResponse = _forgotPasswordResponse.asStateFlow()

    fun sendEmailForPasswordReset() {
        viewModelScope.launch {
            authRepository.sendEmailForPasswordReset(email.value)
                .collectLatest { resource ->
                    _forgotPasswordResponse.value = resource

                    if (resource is Resource.Success) {
                        uiChannel.send(ForgotPasswordUiEvent.PopDialog)
                        uiChannel.send(ForgotPasswordUiEvent.ShowToast(context.getString(R.string.we_sent_you_password_reset_email)))
                    } else if (resource is Resource.Error) {
                        uiChannel.send(ForgotPasswordUiEvent.ShowToast(resource.message ?: ""))
                    }
                }
        }
    }
}

sealed interface ForgotPasswordUiEvent {
    data object PopDialog : ForgotPasswordUiEvent
    data class ShowToast(val message: String) : ForgotPasswordUiEvent
}
