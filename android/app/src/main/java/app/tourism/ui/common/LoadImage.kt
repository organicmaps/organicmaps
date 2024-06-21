package app.tourism.ui.common

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
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
import app.organicmaps.R
import coil.compose.AsyncImage
import coil.decode.SvgDecoder
import coil.request.ImageRequest

@Composable
fun LoadImg(
    url: String?,
    modifier: Modifier = Modifier,
    backgroundColor: Color = MaterialTheme.colorScheme.surface,
    contentScale: ContentScale = ContentScale.Crop
) {
    if (url != null)
        CoilImg(
            modifier = modifier,
            url = url,
            backgroundColor = backgroundColor,
            contentScale = contentScale
        )
    else
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
        ) {
            Image(painter = painterResource(id = R.drawable.image), contentDescription = null)
            Text(text = stringResource(id = R.string.no_image))
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
        modifier = modifier.background(color = backgroundColor),
        model = ImageRequest.Builder(LocalContext.current)
            .data(url)
            .crossfade(500)
            .error(R.drawable.error_centered)
            .build(),
        placeholder = painterResource(R.drawable.placeholder),
        contentDescription = null,
        contentScale = contentScale
    )
}
