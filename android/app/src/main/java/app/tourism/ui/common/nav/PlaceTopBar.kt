package app.tourism.ui.common.nav

import androidx.annotation.DrawableRes
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.common.LoadImg
import app.tourism.ui.theme.TextStyles

@Composable
fun PlaceTopBar(
    modifier: Modifier = Modifier,
    title: String,
    picUrl: String?,
    onBackClick: (() -> Unit)? = null,
    isFavorite: Boolean,
    onFavoriteChanged: (Boolean) -> Unit,
    onMapClick: () -> Unit
) {
    val height = 144.dp
    Box(
        Modifier
            .fillMaxWidth()
            .height(height)
            .clip(
                RoundedCornerShape(
                    topStart = 0.dp,
                    topEnd = 0.dp,
                    bottomStart = 20.dp,
                    bottomEnd = 20.dp
                )
            )
            .then(modifier)
    ) {
        LoadImg(
            modifier = Modifier
                .fillMaxWidth()
                .height(height),
            url = picUrl,
        )

        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(color = Color.Black.copy(alpha = 0.3f)),
        )

        val padding = 16.dp
        Box(
            Modifier
                .fillMaxWidth()
                .align(alignment = Alignment.TopCenter)
                .padding(start = padding, end = padding, top = padding)
        ) {
            onBackClick?.let {
                PlaceTopBarAction(
                    modifier.align(Alignment.CenterStart),
                    iconDrawable = R.drawable.back,
                    onClick = { onBackClick() },
                )
            }
            Row(modifier.align(Alignment.CenterEnd)) {
                PlaceTopBarAction(
                    iconDrawable = if (isFavorite) R.drawable.heart_selected else R.drawable.heart,
                    onClick = { onFavoriteChanged(!isFavorite) },
                )
                PlaceTopBarAction(
                    iconDrawable = R.drawable.map,
                    onClick = onMapClick,
                )
            }
        }

        Text(
            modifier = Modifier
                .align(Alignment.BottomStart)
                .padding(start = padding, end = padding, bottom = padding),
            text = title,
            style = TextStyles.h2,
            color = Color.White
        )
    }
}

@Composable
private fun PlaceTopBarAction(
    modifier: Modifier = Modifier,
    @DrawableRes iconDrawable: Int,
    onClick: () -> Unit,
) {
    val shape = CircleShape
    IconButton(modifier = Modifier.then(modifier), onClick = onClick) {
        Icon(
            modifier = Modifier
                .clickable { onClick() }
                .background(color = Color.White.copy(alpha = 0.2f), shape = shape)
                .clip(shape)
                .size(40.dp)
                .padding(8.dp),
            painter = painterResource(id = iconDrawable),
            tint = Color.White,
            contentDescription = null,
        )
    }
}
