package app.tourism.ui.screens.main.place_details.reviews.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.IntrinsicSize
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
import androidx.compose.foundation.layout.wrapContentSize
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
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.details.User
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.LoadImg
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.special.CountryAsLabel
import app.tourism.ui.common.special.CountryFlag
import app.tourism.ui.common.special.RatingBar
import app.tourism.ui.screens.main.place_details.gallery.imageShape
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getHintColor

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun Review(
    modifier: Modifier = Modifier,
    review: Review,
    onMoreClick: (picsUrls: List<String>) -> Unit,
    onDeleteClick: (() -> Unit)? = null,
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
            if (review.deletionPlanned) {
                Text(stringResource(id = R.string.deletionPlanned))
            } else {
                review.date?.let {
                    Text(text = it, style = TextStyles.b2, color = getHintColor())
                }
            }
        }
        VerticalSpace(height = 16.dp)

        RatingBar(
            rating = review.rating,
            size = 24.dp,
        )
        VerticalSpace(height = 16.dp)

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

        if(!review.comment.isNullOrBlank()) {
            Comment(comment = review.comment)
            VerticalSpace(height = 16.dp)
        }

        onDeleteClick?.let {
            TextButton(
                onClick = { onDeleteClick() },
            ) {
                Text(text = stringResource(id = R.string.delete_review))
            }
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
        HorizontalSpace(width = 12.dp)
        Row(modifier = Modifier) {
            Column {
                VerticalSpace(3.dp)
                CountryFlag(
                    modifier = Modifier.width(50.dp),
                    countryCodeName = user.countryCodeName,
                )
            }
            Text(
                modifier = Modifier.weight(1f).width(IntrinsicSize.Min),
                text = user.name,
                style = TextStyles.h4,
                fontWeight = FontWeight.W600,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}

@Composable
fun Comment(modifier: Modifier = Modifier, comment: String) {
    var hasOverflown by remember { mutableStateOf(false) }
    var expanded by remember { mutableStateOf(false) }

    val shape = RoundedCornerShape(20.dp)
    val onClick = { expanded = !expanded }
    Column(
        Modifier
            .fillMaxWidth()
            .background(color = MaterialTheme.colorScheme.surface, shape = shape)
            .clip(shape)
            .clickable { onClick() }
            .padding(start = 16.dp, end = 16.dp, top = 16.dp)
            .then(modifier),
    ) {
        Text(
            text = comment,
            style = TextStyles.h4.copy(fontWeight = FontWeight.W400),
            maxLines = if (expanded) 6969 else 2,
            overflow = TextOverflow.Ellipsis,
            onTextLayout = {
                if (it.hasVisualOverflow) hasOverflown = true
            }
        )
        if (hasOverflown) {
            TextButton(onClick = { onClick() }, contentPadding = PaddingValues(0.dp)) {
                Text(text = stringResource(id = if (expanded) R.string.less else R.string.more))
            }
        } else {
            VerticalSpace(height = 16.dp)
        }
    }
}


@Composable
fun ReviewPic(modifier: Modifier = Modifier, url: String) {
    LoadImg(
        modifier = Modifier
            .width(73.dp)
            .height(65.dp)
            .clip(imageShape)
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
        .clip(imageShape)

val imageShape = RoundedCornerShape(4.dp)