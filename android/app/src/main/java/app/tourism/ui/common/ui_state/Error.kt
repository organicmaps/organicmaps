package app.tourism.ui.common.ui_state

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign.Companion.Center
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.theme.TextStyles

@Composable
fun Error(
    modifier: Modifier = Modifier,
    errorMessage: String? = null,
    status: Boolean = true,
    onEntireScreen: Boolean = true,
    onRetry: (() -> Unit)? = null
) {
    if (status) {
        Column(
            modifier = if (onEntireScreen) modifier
                .fillMaxSize()
                .padding(Constants.SCREEN_PADDING) else modifier.padding(Constants.SCREEN_PADDING),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            if (onEntireScreen)
                Icon(
                    modifier = Modifier
                        .size(64.dp),
                    painter = painterResource(id = R.drawable.error),
                    tint = MaterialTheme.colorScheme.primary,
                    contentDescription = null
                )

            Spacer(modifier = Modifier.size(16.dp))

            Text(
                text = errorMessage
                    ?: stringResource(id = if (onEntireScreen) R.string.no_network else R.string.smth_went_wrong),
                style = TextStyles.h3,
                textAlign = Center
            )

            if (onRetry != null)
                if (onEntireScreen) {
                    Spacer(modifier = Modifier.size(16.dp))
                    PrimaryButton(
                        label = stringResource(id = R.string.retry),
                        onClick = { onRetry.invoke() }
                    )
                } else {
                    IconButton(onClick = { onRetry() }) {
                        Icon(
                            painter = painterResource(id = R.drawable.baseline_refresh_24),
                            tint = MaterialTheme.colorScheme.primary,
                            contentDescription = null
                        )
                    }
                }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun NetworkError_preview() {
    Column {
        Error(status = true, onEntireScreen = false) {}
        VerticalSpace(height = 16.dp)
        Error(status = true) {}
    }
}
