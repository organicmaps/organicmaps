package app.tourism.ui.screens.main.place_details.reviews.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.details.User
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.LoadImg
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.special.CountryAsLabel
import app.tourism.ui.common.special.RatingBar
import app.tourism.ui.screens.main.place_details.gallery.imageShape
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getHintColor

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun Review(
    modifier: Modifier = Modifier,
    review: Review,
    onMoreClick: (picsUrls: List<String>) -> Unit
) {
    Column {
        HorizontalDivider(color = MaterialTheme.colorScheme.surface)
        VerticalSpace(height = 16.dp)
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .then(modifier),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            User(modifier = Modifier.weight(1f), user = review.user)
            review.date?.let {
                Text(text = it, style = TextStyles.b2, color = getHintColor())
            }
        }
        VerticalSpace(height = 16.dp)

        review.rating?.let {
            RatingBar(
                rating = it.toFloat(),
                size = 24.dp,
            )
            VerticalSpace(height = 16.dp)
        }

        val maxPics = 3
        val theresMore = review.picsUrls.size > maxPics
        val first3pics = if (theresMore) review.picsUrls.take(3) else review.picsUrls
        if (first3pics.isNotEmpty()) {
            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                first3pics.forEachIndexed { index, url ->
                    if (index == maxPics - 1 && theresMore) {
                        ShowMore(
                            url = url,
                            onClick = {
                                onMoreClick(review.picsUrls)
                            },
                            remaining = review.picsUrls.size - 3
                        )
                    } else {
                        ReviewPic(url = url)
                    }
                }
            }
            VerticalSpace(height = 16.dp)
        }

        review.comment?.let {
            Comment(comment = it)
            VerticalSpace(height = 16.dp)
        }
    }
}


@Composable
fun User(modifier: Modifier = Modifier, user: User) {
    Row(
        modifier = Modifier
            .size(66.dp)
            .then(modifier),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        LoadImg(
            modifier = Modifier
                .fillMaxHeight()
                .aspectRatio(1f)
                .clip(CircleShape),
            url = user.pfpUrl,
        )
        HorizontalSpace(width = 8.dp)
        Column {

            Text(text = user.name, style = TextStyles.h4, fontWeight = FontWeight.W600)
            user.countryCodeName?.let {
                CountryAsLabel(
                    Modifier.fillMaxWidth(),
                    user.countryCodeName,
                    contentColor = MaterialTheme.colorScheme.onBackground.toArgb(),
                )
            }
        }
    }
}

@Composable
fun Comment(modifier: Modifier = Modifier, comment: String) {
    var expanded by remember { mutableStateOf(false) }

    val shape = RoundedCornerShape(20.dp)
    val onClick = { expanded = !expanded }
    Column(
        Modifier
            .fillMaxWidth()
            .background(color = MaterialTheme.colorScheme.surface, shape = shape)
            .clip(shape)
            .clickable { onClick() }
            .padding(
                start = Constants.SCREEN_PADDING,
                end = Constants.SCREEN_PADDING,
                top = Constants.SCREEN_PADDING,
            )
            .then(modifier),
    ) {
        Text(
            text = comment,
            style = TextStyles.h4.copy(fontWeight = FontWeight.W400),
            maxLines = if (expanded) 6969 else 2,
            overflow = TextOverflow.Ellipsis,
        )
        TextButton(onClick = { onClick() }, contentPadding = PaddingValues(0.dp)) {
            Text(text = stringResource(id = if (expanded) R.string.less else R.string.more))
        }
    }
}


@Composable
fun ReviewPic(modifier: Modifier = Modifier, url: String) {
    LoadImg(
        modifier = Modifier
            .width(73.dp)
            .height(65.dp)
            .clip(RoundedCornerShape(4.dp))
            .then(modifier),
        url = url,
    )
}

@Composable
fun ShowMore(url: String, onClick: () -> Unit, remaining: Int) {
    Box(
        modifier = Modifier
            .clickable { onClick() }
            .getImageProperties(),
        contentAlignment = Alignment.Center
    ) {
        ReviewPic(url = url)
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    color = Color.Black.copy(alpha = 0.5f),
                    shape = imageShape
                ),
        )
        Text(
            text = "+$remaining",
            style = TextStyles.h3,
            color = Color.White,
        )
    }
}

@Composable
fun Modifier.getImageProperties() =
    this
        .width(73.dp)
        .height(65.dp)
        .clip(RoundedCornerShape(4.dp))