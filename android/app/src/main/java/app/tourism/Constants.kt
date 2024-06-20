package app.tourism

import androidx.compose.foundation.border
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R

const val TAG = "GLOBAL_TAG"
const val BASE_URL = "http://192.168.1.80:8888/api/"

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
        "https://cdn.pixabay.com/photo/2020/03/24/22/34/illustration-4965674_960_720.jpg"
    const val LOGO_URL_EXAMPLE = "https://brandeps.com/logo-download/O/OSCE-logo-vector-01.svg"

}

@Composable
fun Modifier.applyAppBorder() = this.border(
    width = 1.dp,
    color = colorResource(id = R.color.border),
    shape = RoundedCornerShape(20.dp)
).clip(RoundedCornerShape(20.dp))
