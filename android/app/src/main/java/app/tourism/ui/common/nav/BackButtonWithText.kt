package app.tourism.ui.common.nav

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.common.HorizontalSpace

@Composable
fun BackButtonWithText(modifier: Modifier = Modifier, onBackClick: () -> Unit) {
    TextButton(
        modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
        onClick = { onBackClick() },
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(
                painter = painterResource(id = R.drawable.back),
                contentDescription = stringResource(id = R.string.back)
            )
            HorizontalSpace(width = 16.dp)
            Text(text = stringResource(id = R.string.back))
        }
    }
}