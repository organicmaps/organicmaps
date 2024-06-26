package app.tourism.ui.screens.main.place_details.gallery

import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp

@Composable
fun Modifier.propertiesForSmallImage() =
    this
        .height(150.dp)
        .clip(imageShape)

val imageShape = RoundedCornerShape(10.dp)