package app.tourism.ui.screens.auth.sign_in

import PasswordEditText
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
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
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.BASE_URL
import app.tourism.Constants
import app.tourism.domain.models.resource.Resource
import app.tourism.drawOverlayForTextBehind
import app.tourism.ui.ObserveAsEvents
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.nav.BackButton
import app.tourism.ui.common.textfields.AuthEditText
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.utils.showToast
import app.tourism.utils.openUrlInBrowser

@Composable
fun SignInScreen(
    onSignInComplete: () -> Unit,
    onBackClick: () -> Unit,
    vm: SignInViewModel = hiltViewModel(),
) {
    val context = LocalContext.current
    val focusManager = LocalFocusManager.current

    val userName = vm.email.collectAsState().value
    val password = vm.password.collectAsState().value

    val signInResponse = vm.signInResponse.collectAsState().value

    ObserveAsEvents(flow = vm.uiEventsChannelFlow) { event ->
        when (event) {
            is UiEvent.NavigateToMainActivity -> onSignInComplete()
            is UiEvent.ShowToast -> context.showToast(event.message)
        }
    }

    Box(modifier = Modifier.fillMaxSize()) {
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
                .align(Alignment.TopCenter)
        ) {
            VerticalSpace(height = 80.dp)
            Box(Modifier.padding(Constants.SCREEN_PADDING)) {
                Image(
                    painter = painterResource(id = R.drawable.blur_background),
                    contentDescription = null
                )
                Column(Modifier.padding(36.dp)) {
                    Text(
                        modifier = Modifier.align(Alignment.CenterHorizontally),
                        text = stringResource(id = R.string.sign_in_title),
                        style = TextStyles.h2,
                        color = Color.White
                    )
                    VerticalSpace(height = 32.dp)
                    AuthEditText(
                        value = userName,
                        onValueChange = { vm.setEmail(it) },
                        hint = stringResource(id = R.string.email),
                        keyboardActions = KeyboardActions(
                            onNext = {
                                focusManager.moveFocus(FocusDirection.Next)
                            },
                        ),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                    )
                    VerticalSpace(height = 32.dp)
                    PasswordEditText(
                        value = password,
                        onValueChange = { vm.setPassword(it) },
                        hint = stringResource(id = R.string.password),
                        keyboardActions = KeyboardActions(onDone = { onSignInComplete() }),
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                    )
                    VerticalSpace(height = 48.dp)
                    PrimaryButton(
                        modifier = Modifier.fillMaxWidth(),
                        label = stringResource(id = R.string.sign_in),
                        isLoading = signInResponse is Resource.Loading,
                        onClick = { vm.signIn() },
                    )
                    VerticalSpace(height = 16.dp)

                    TextButton(
                        onClick = {
                            openUrlInBrowser(
                                context,
                                "$BASE_URL/forgot-password"
                            )
                        },
                    ) {
                        Text(
                            text = stringResource(id = R.string.forgot_password),
                            color = Color.White
                        )
                    }
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

