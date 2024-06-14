package app.tourism.ui.common.nav

import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R

@Composable
fun BackButton(
    modifier: Modifier = Modifier,
    onBackClick: () -> Boolean,
    tint: Color = MaterialTheme.colorScheme.onBackground
) {
    IconButton(
        modifier = Modifier.padding(12.dp).then(modifier),
        onClick = { onBackClick() }
    ) {
        Icon(
            modifier = Modifier.size(28.dp),
            painter = painterResource(id = R.drawable.back),
            tint = tint,
            contentDescription = null
        )
    }
}
