package app.tourism.ui.screens.main.profile.personal_data

import android.view.LayoutInflater
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.domain.models.resource.Resource
import app.tourism.ui.ObserveAsEvents
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.ImagePicker
import app.tourism.ui.common.SpaceForNavBar
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.textfields.AppEditText
import app.tourism.ui.screens.main.profile.profile.ProfileViewModel
import app.tourism.ui.screens.main.profile.profile.UiEvent
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.utils.showToast
import app.tourism.utils.FileUtils
import coil.compose.AsyncImage
import com.hbb20.CountryCodePicker
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.File

@Composable
fun PersonalDataScreen(onBackClick: () -> Unit, profileVM: ProfileViewModel) {
    val context = LocalContext.current
    val focusManager = LocalFocusManager.current
    val coroutineScope = rememberCoroutineScope()

    val personalData = profileVM.profileDataResource.collectAsState().value
    val pfpFile = profileVM.pfpFile.collectAsState().value
    val fullName = profileVM.fullName.collectAsState().value
    val email = profileVM.email.collectAsState().value
    val countryCodeName = profileVM.countryCodeName.collectAsState().value

    ObserveAsEvents(flow = profileVM.uiEventsChannelFlow) { event ->
        if (event is UiEvent.ShowToast) context.showToast(event.message)
    }

    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.personal_data),
                onBackClick = onBackClick,
            )
        },
        contentWindowInsets = Constants.USUAL_WINDOW_INSETS
    ) { paddingValues ->
        if (personalData is Resource.Success && personalData.data != null) {
            val data = personalData.data
            Column(
                Modifier
                    .padding(paddingValues)
                    .verticalScroll(rememberScrollState())
            ) {
                VerticalSpace(height = 32.dp)
                Row(
                    Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    AsyncImage(
                        modifier = Modifier
                            .size(100.dp)
                            .clip(CircleShape),
                        model = pfpFile,
                        contentScale = ContentScale.Crop,
                        contentDescription = null,
                    )
                    HorizontalSpace(width = 20.dp)
                    ImagePicker(
                        showPreview = false,
                        onSuccess = { uri ->
                            coroutineScope.launch(Dispatchers.IO) {
                                profileVM.setPfpFile(
                                    File(FileUtils(context).getPath(uri))
                                )
                            }
                        }
                    ) {
                        Row(
                            modifier = Modifier.padding(16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            val uploadPhotoText = stringResource(id = R.string.upload_photo)
                            Icon(
                                painter = painterResource(id = R.drawable.image_down),
                                contentDescription = uploadPhotoText,
                            )
                            HorizontalSpace(width = 8.dp)
                            Text(text = uploadPhotoText, style = TextStyles.h4)
                        }
                    }

                }
                VerticalSpace(height = 24.dp)

                AppEditText(
                    value = fullName, onValueChange = { profileVM.setFullName(it) },
                    hint = stringResource(id = R.string.full_name),
                    keyboardActions = KeyboardActions(
                        onNext = {
                            focusManager.moveFocus(FocusDirection.Next)
                        },
                    ),
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                )
                SpaceBetweenTextFields()

                AppEditText(
                    value = email, onValueChange = { profileVM.setEmail(it) },
                    hint = stringResource(id = R.string.email),
                    keyboardActions = KeyboardActions(
                        onNext = {
                            focusManager.moveFocus(FocusDirection.Next)
                        },
                    ),
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Next),
                )
                SpaceBetweenTextFields()

                Text(
                    text = stringResource(id = R.string.country),
                    fontSize = 12.sp
                )
                val lContentColor = MaterialTheme.colorScheme.onBackground.toArgb()
                AndroidView(
                    factory = { context ->
                        val view = LayoutInflater.from(context)
                            .inflate(R.layout.ccp_profile, null, false)
                        val ccp = view.findViewById<CountryCodePicker>(R.id.ccp)

                        ccp.apply {
                            setDialogBackgroundColor(Color.Transparent.toArgb())
                            this.contentColor = lContentColor
                            setDialogTextColor(lContentColor)
                            setArrowColor(lContentColor)

                            setCountryForNameCode(countryCodeName)
                            setOnCountryChangeListener {
                                profileVM.setCountryCodeName(ccp.selectedCountryNameCode)
                            }
                        }
                        view
                    }
                )
                VerticalSpace(height = 10.dp)
                HorizontalDivider(
                    modifier = Modifier.fillMaxWidth(),
                    color = MaterialTheme.colorScheme.onBackground,
                    thickness = 1.dp
                )
                VerticalSpace(height = 48.dp)

                PrimaryButton(
                    label = stringResource(id = R.string.save),
                    onClick = { profileVM.save() },
                )

                SpaceForNavBar()
            }
        }
    }
}

@Composable
fun ColumnScope.SpaceBetweenTextFields() {
    VerticalSpace(height = 24.dp)
}