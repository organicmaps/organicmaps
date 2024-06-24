package app.tourism.ui.screens.auth.welcome

import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.drawOverlayForTextBehind
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.theme.TextStyles

@Composable
fun WelcomeScreen(
    onLanguageClicked: () -> Unit,
    onSignInClicked: () -> Unit,
    onSignUpClicked: () -> Unit,
) {
    Box(modifier = Modifier.fillMaxSize()) {
        Image(
            modifier = Modifier.fillMaxSize(),
            painter = painterResource(id = R.drawable.splash_background),
            contentScale = ContentScale.Crop,
            contentDescription = null
        )

        Row(
            Modifier
                .align(Alignment.TopCenter)
                .padding(top = 16.dp)
                .clickable {
                    onLanguageClicked()
                },
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(text = stringResource(id = R.string.current_language), color = Color.White)
            HorizontalSpace(width = 8.dp)
            Icon(
                painter = painterResource(id = R.drawable.globe),
                tint = Color.White,
                contentDescription = null,
            )
        }

        Column(
            Modifier
                .align(Alignment.BottomStart)
                .drawOverlayForTextBehind()
                .padding(Constants.SCREEN_PADDING)
        ) {
            Text(
                text = stringResource(id = R.string.welcome_to_tjk),
                color = Color.White,
                style = TextStyles.humongous.copy()
            )
            VerticalSpace(height = 24.dp)
            Row(Modifier.fillMaxWidth()) {
                PrimaryButton(
                    modifier = Modifier.weight(1f),
                    label = stringResource(id = R.string.sign_in),
                    onClick = { onSignInClicked() },
                )
                HorizontalSpace(width = 16.dp)
                PrimaryButton(
                    modifier = Modifier.weight(1f),
                    label = stringResource(id = R.string.sign_up),
                    onClick = { onSignUpClicked() },
                )
            }
            VerticalSpace(height = 24.dp)
            Row(
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "©",
                    color = Color.White,
                    style = TextStyles.h1.copy()
                )
                HorizontalSpace(width = 8.dp)
                Text(
                    text = stringResource(id = R.string.organization_name),
                    color = Color.White,
                    style = TextStyles.h4.copy()
                )
            }
            VerticalSpace(height = 36.dp)
        }
    }
}