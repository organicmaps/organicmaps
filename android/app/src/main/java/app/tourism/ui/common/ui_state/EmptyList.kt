package app.tourism.ui.common.ui_state

import androidx.compose.foundation.layout.Box
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import app.organicmaps.R
import app.tourism.ui.theme.TextStyles


@Composable
fun EmptyList(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier,
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = stringResource(id = R.string.empty_list),
            style = TextStyles.h2
        )
    }
}