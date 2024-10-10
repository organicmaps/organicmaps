package app.tourism.ui.screens.auth.sign_up

import PasswordEditText
import android.view.LayoutInflater
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.domain.models.resource.Resource
import app.tourism.drawOverlayForTextBehind
import app.tourism.ui.ObserveAsEvents
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.nav.BackButton
import app.tourism.ui.common.textfields.AuthEditText
import app.tourism.ui.screens.auth.navigateToMainActivity
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.utils.showToast
import app.tourism.utils.openUrlInBrowser
import com.hbb20.CountryCodePicker

@Composable
fun SignUpScreen(
    onSignUpComplete: () -> Unit,
    onBackClick: () -> Unit,
    vm: SignUpViewModel = hiltViewModel(),
) {
    val context = LocalContext.current
    val focusManager = LocalFocusManager.current

    val registrationData = vm.registrationData.collectAsState().value
    val fullName = registrationData?.fullName
    var countryNameCode = registrationData?.country
    val email = registrationData?.email
    val password = registrationData?.password
    val confirmPassword = registrationData?.passwordConfirmation

    val signUpResponse = vm.signUpResponse.collectAsState().value

    ObserveAsEvents(flow = vm.uiEventsChannelFlow) { event ->
        when (event) {
            is UiEvent.NavigateToMainActivity -> navigateToMainActivity(context)
            is UiEvent.ShowToast -> context.showToast(event.message)
        }
    }

    Box(
        modifier = Modifier.fillMaxSize()
    ) {
        Image(
            modifier = Modifier.fillMaxSize(),
            painter = painterResource(id = R.drawable.splash_background),
            contentScale = ContentScale.Crop,
            contentDescription = null
        )

        Box(Modifier.padding(Constants.SCREEN_PADDING)) {
            BackButton(
                modifier = Modifier.align(Alignment.TopStart),
                onBackClick = onBackClick,
                tint = Color.White
            )
        }

        Column(
            Modifier
                .fillMaxWidth()
                .align(alignment = Alignment.TopCenter)
        ) {
            VerticalSpace(height = 48.dp)
            Box(Modifier.padding(Constants.SCREEN_PADDING)) {
                Image(
                    painter = painterResource(id = R.drawable.blur_background),
                    contentDescription = null
                )
                Column(
                    Modifier.padding(36.dp)
                ) {
                    Text(
                        modifier = Modifier.align(Alignment.CenterHorizontally),
                        text = stringResource(id = R.string.sign_up_title),
                        style = TextStyles.h2,
                        color = Color.White
                    )
                    VerticalSpace(height = 16.dp)
                    AuthEditText(
                        value = fullName ?: "",
                        onValueChange = { vm.setFullName(it) },
                        hint = stringResource(id = R.string.full_name),
                        keyboardActions = KeyboardActions(
                            onNext = {
                                focusManager.moveFocus(FocusDirection.Next)
                            },
                        ),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                    )
                    VerticalSpace(height = 16.dp)
                    AndroidView(
                        factory = { context ->
                            val view = LayoutInflater.from(context)
                                .inflate(R.layout.ccp_auth, null, false)
                            val ccp = view.findViewById<CountryCodePicker>(R.id.ccp)
                            ccp.setCountryForNameCode("TJ")
                            ccp.setOnCountryChangeListener {
                                vm.setCountryNameCode(ccp.selectedCountryNameCode)
                            }
                            view
                        })
                    HorizontalDivider(
                        modifier = Modifier.fillMaxWidth(),
                        color = Color.White,
                        thickness = 1.dp
                    )
                    VerticalSpace(height = 16.dp)
                    AuthEditText(
                        value = email ?: "",
                        onValueChange = { vm.setEmail(it) },
                        hint = stringResource(id = R.string.email),
                        keyboardActions = KeyboardActions(
                            onNext = {
                                focusManager.moveFocus(FocusDirection.Next)
                            },
                        ),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                    )
                    VerticalSpace(height = 16.dp)
                    PasswordEditText(
                        value = password ?: "",
                        onValueChange = { vm.setPassword(it) },
                        hint = stringResource(id = R.string.password),
                        keyboardActions = KeyboardActions(
                            onNext = {
                                focusManager.moveFocus(FocusDirection.Next)
                            },
                        ),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                    )
                    VerticalSpace(height = 16.dp)
                    PasswordEditText(
                        value = confirmPassword ?: "",
                        onValueChange = { vm.setConfirmPassword(it) },
                        hint = stringResource(id = R.string.confirm_password),
                        keyboardActions = KeyboardActions(onDone = { vm.signUp() }),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                    )
                    VerticalSpace(height = 48.dp)
                    PrimaryButton(
                        modifier = Modifier.fillMaxWidth(),
                        label = stringResource(id = R.string.sign_up),
                        isLoading = signUpResponse is Resource.Loading,
                        onClick = { vm.signUp() },
                    )
                }
            }
        }

        Text(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.BottomStart)
                .drawOverlayForTextBehind()
                .padding(Constants.SCREEN_PADDING)
                .clickable { openUrlInBrowser(context = context, url = "https://rebus.tj") },
            text = stringResource(id = R.string.developed_by_label),
            textAlign = TextAlign.End,
            color = Color.White,
            style = TextStyles.h4.copy()
        )
    }
}

