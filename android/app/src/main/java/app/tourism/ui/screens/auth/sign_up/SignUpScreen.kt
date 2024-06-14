package app.tourism.ui.screens.auth.sign_up

import PasswordEditText
import android.view.LayoutInflater
import androidx.compose.foundation.Image
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.BlurryContainer
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.nav.BackButton
import app.tourism.ui.common.textfields.AuthEditText
import app.tourism.ui.theme.TextStyles
import com.hbb20.CountryCodePicker

@Composable
fun SignUpScreen(
    onSignUpClicked: () -> Unit,
    onBackClick: () -> Boolean,
) {
    val focusManager = LocalFocusManager.current

    val fullName = remember { mutableStateOf("") }
    var countryNameCode by remember { mutableStateOf("") }
    val username = remember { mutableStateOf("") }
    val password = remember { mutableStateOf("") }
    val confirmPassword = remember { mutableStateOf("") }

    Box(modifier = Modifier.fillMaxSize()) {
        Image(
            modifier = Modifier.fillMaxSize(),
            painter = painterResource(id = R.drawable.splash_background),
            contentScale = ContentScale.Crop,
            contentDescription = null
        )

        BackButton(
            modifier = Modifier.align(Alignment.TopStart),
            onBackClick = onBackClick,
            tint = Color.White
        )

        BlurryContainer(
            Modifier
                .align(Alignment.Center)
                .fillMaxWidth()
                .padding(Constants.SCREEN_PADDING),
        ) {
            Column(
                Modifier.padding(36.dp)
            ) {
                Text(
                    modifier = Modifier.align(Alignment.CenterHorizontally),
                    text = stringResource(id = R.string.sign_up_title),
                    style = TextStyles.h2,
                    color = Color.White
                )
                VerticalSpace(height = 32.dp)
                AuthEditText(
                    value = fullName,
                    hint = stringResource(id = R.string.full_name),
                    keyboardActions = KeyboardActions(
                        onNext = {
                            focusManager.moveFocus(FocusDirection.Next)
                        },
                    ),
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                )
                VerticalSpace(height = 32.dp)
                AndroidView(
                    factory = { context ->
                        val view = LayoutInflater.from(context)
                            .inflate(R.layout.country_code_picker, null, false)
                        val ccp = view.findViewById<CountryCodePicker>(R.id.ccp)
                        ccp.setCountryForNameCode("TJ")
                        ccp.setOnCountryChangeListener {
                            countryNameCode = ccp.selectedCountryNameCode
                        }
                        view
                    })
                HorizontalDivider(
                    modifier = Modifier.fillMaxWidth(),
                    color = Color.White,
                    thickness = 1.dp
                )
                VerticalSpace(height = 32.dp)
                AuthEditText(
                    value = username,
                    hint = stringResource(id = R.string.username),
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
                    hint = stringResource(id = R.string.password),
                    keyboardActions = KeyboardActions(
                        onNext = {
                            focusManager.moveFocus(FocusDirection.Next)
                        },
                    ),
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                )
                VerticalSpace(height = 32.dp)
                PasswordEditText(
                    value = confirmPassword,
                    hint = stringResource(id = R.string.confirm_password),
                    keyboardActions = KeyboardActions(onDone = { onSignUpClicked() }),
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                )
                VerticalSpace(height = 48.dp)
                PrimaryButton(
                    modifier = Modifier.fillMaxWidth(),
                    label = stringResource(id = R.string.sign_up),
                    onClick = { onSignUpClicked() },
                )
            }
        }
    }
}

