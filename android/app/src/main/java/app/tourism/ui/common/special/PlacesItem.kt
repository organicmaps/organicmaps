package app.tourism.ui.common.special

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.applyAppBorder
import app.tourism.domain.models.common.PlaceShort
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.LoadImg
import app.tourism.ui.theme.HeartRed
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getStarColor
import app.tourism.utils.getAnnotatedStringFromHtml

@Composable
fun PlacesItem(
    modifier: Modifier = Modifier,
    place: PlaceShort,
    onPlaceClick: () -> Unit,
    isFavorite: Boolean,
    onFavoriteChanged: (Boolean) -> Unit
) {
    val height = 130.dp
    val shape = RoundedCornerShape(20.dp)
    Row(
        Modifier
            .fillMaxWidth()
            .height(height)
            .applyAppBorder()
            .clip(shape)
            .clickable { onPlaceClick() }
            .then(modifier)
    ) {
        LoadImg(modifier = Modifier
            .size(height)
            .clip(shape), url = place.pic)
        Column(
            Modifier
                .fillMaxHeight(0.9f)
                .padding(8.dp),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    modifier = Modifier.weight(1f, fill = true),
                    text = place.name,
                    style = TextStyles.h3,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )

                IconButton(
                    modifier = Modifier.size(28.dp),
                    onClick = {
                        onFavoriteChanged(!isFavorite)
                    },
                ) {
                    Icon(
                        painterResource(id = if (isFavorite) R.drawable.heart_selected else R.drawable.heart),
                        contentDescription = stringResource(id = R.string.add_to_favorites),
                        tint = HeartRed,
                    )
                }
            }

            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(text = "%.1f".format(place.rating), style = TextStyles.b1)
                HorizontalSpace(width = 8.dp)
                Icon(
                    modifier = Modifier.size(12.dp),
                    painter = painterResource(id = R.drawable.star),
                    contentDescription = null,
                    tint = getStarColor(),
                )
            }

            place.excerpt?.let {
                Text(
                    text = it.getAnnotatedStringFromHtml(),
                    style = TextStyles.b1,
                    maxLines = 3,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
    }
}