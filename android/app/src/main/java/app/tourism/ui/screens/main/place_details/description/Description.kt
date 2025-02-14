package app.tourism.ui.screens.main.place_details.description

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.utils.enableLocation
import app.tourism.utils.getAnnotatedStringFromHtml

@Composable
fun DescriptionScreen(
    description: String?,
    onCreateRoute: (() -> Unit)?,
) {
    val context = LocalContext.current
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = Constants.SCREEN_PADDING)
    ) {
        description?.let {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                VerticalSpace(height = 16.dp)
                Text(
                    text = it.getAnnotatedStringFromHtml(),
                    style = TextStyles.b1,
                    fontSize = 14.sp
                )
                VerticalSpace(height = 100.dp)
            }
        }

        onCreateRoute?.let {
            PrimaryButton(
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .offset(y = (-32).dp),
                label = stringResource(id = R.string.show_route),
                onClick = {
                    enableLocation(
                        context = context,
                        onSuccess = {
                            onCreateRoute()
                        },
                    )
                },
            )
        }
    }
}
