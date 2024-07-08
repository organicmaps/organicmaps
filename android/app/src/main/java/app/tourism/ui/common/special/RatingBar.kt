package app.tourism.ui.common.special

import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Icon
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.theme.getStarColor

@Composable
fun RatingBar(
    rating: Int,
    size: Dp = 30.dp,
    maxRating: Int = 5,
    onRatingChanged: ((Float) -> Unit)? = null,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(2.dp)) {
        for (i in 1..maxRating) {
            Icon(
                modifier = Modifier
                    .size(size)
                    .clickable(
                        indication = null,
                        interactionSource = remember { MutableInteractionSource() },
                        onClick = {
                            onRatingChanged?.invoke(i.toFloat())
                        },
                    ),
                painter =
                painterResource(id = if (i <= rating) R.drawable.star else R.drawable.star_border),
                contentDescription = null,
                tint = getStarColor()
            )
        }
    }
}