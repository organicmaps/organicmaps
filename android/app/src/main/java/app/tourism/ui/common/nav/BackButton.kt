package app.tourism.ui.common.nav

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Icon
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
    onBackClick: () -> Unit,
    tint: Color = MaterialTheme.colorScheme.onBackground
) {
    Icon(
        modifier = Modifier
            .size(30.dp)
            .clickable { onBackClick() }
            .then(modifier),
        painter = painterResource(id = R.drawable.back),
        tint = tint,
        contentDescription = null
    )

}
