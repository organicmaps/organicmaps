package app.tourism.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import app.organicmaps.R
import app.tourism.ui.theme.TextStyles
import coil.compose.AsyncImage
import coil.request.ImageRequest

@Composable
fun LoadImg(
    url: String?,
    modifier: Modifier = Modifier,
    backgroundColor: Color = Color.Transparent,
    contentScale: ContentScale = ContentScale.Crop
) {
    if (!url.isNullOrBlank())
        CoilImg(
            modifier = modifier,
            url = url,
            backgroundColor = backgroundColor,
            contentScale = contentScale
        )
    else
        Column(
            Modifier
                .background(color = MaterialTheme.colorScheme.surface, shape = CircleShape)
                .then(modifier),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
        ) {
            Text(
                text = stringResource(id = R.string.no_image),
                style = TextStyles.b3,
                textAlign = TextAlign.Center
            )
        }
}

@Composable
fun CoilImg(
    modifier: Modifier = Modifier,
    url: String,
    backgroundColor: Color,
    contentScale: ContentScale
) {
    AsyncImage(
        modifier = Modifier
            .background(color = backgroundColor)
            .then(modifier)
            .fillMaxSize(),
        model = ImageRequest.Builder(LocalContext.current)
            .data(url)
            .crossfade(200)
            .error(R.drawable.error_centered)
            .build(),
        placeholder = painterResource(R.drawable.placeholder),
        contentDescription = null,
        contentScale = contentScale
    )
}
