package app.tourism

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.theme.getBorderColor

const val TAG = "GLOBAL_TAG"
const val BASE_URL = "https://tourismmap.tj"

object Constants {
    // UI
    val SCREEN_PADDING = 16.dp
    val USUAL_WINDOW_INSETS = WindowInsets(
        left = SCREEN_PADDING,
        right = SCREEN_PADDING,
        bottom = 0.dp,
        top = 0.dp
    )

    // image loading
    const val IMAGE_URL_EXAMPLE =
        "https://img.freepik.com/free-photo/young-woman-hiker-taking-photo-with-smartphone-on-mountains-peak-in-winter_335224-427.jpg?w=2000"
    const val THUMBNAIL_URL_EXAMPLE =
        "https://render.fineartamerica.com/images/images-profile-flow/400/images-medium-large-5/awesome-solitude-bess-hamiti.jpg"
    const val LOGO_URL_EXAMPLE = "https://brandeps.com/logo-download/O/OSCE-logo-vector-01.svg"

    // data
    val categories = mapOf(
        "sights" to R.string.sights,
        "restaurants" to R.string.restaurants,
        "hotels_tourism" to R.string.hotels_tourism,
    )
}

@Composable
fun Modifier.applyAppBorder() = this
    .border(
        width = 1.dp,
        color = getBorderColor(),
        shape = RoundedCornerShape(20.dp)
    )
    .clip(RoundedCornerShape(20.dp))

@Composable
fun Modifier.drawOverlayForTextBehind() =
    this.background(
        brush = Brush.verticalGradient(
            colors = listOf(
                Color.Transparent,
                Color.Black.copy(alpha = 0.9f),
            )
        )
    )

@Composable
fun Modifier.drawDarkContainerBehind(): Modifier {
    val alpha = 0.4f
    return this
        .clip(RoundedCornerShape(20.dp))
        .background(
            Brush.verticalGradient(
                colors = listOf(
                    Color.Black.copy(alpha = alpha),
                    Color.Black.copy(alpha = alpha)
                )
            )
        )
}
