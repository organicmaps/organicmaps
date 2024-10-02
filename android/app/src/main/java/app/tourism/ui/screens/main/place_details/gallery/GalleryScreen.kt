package app.tourism.ui.screens.main.place_details.gallery

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import app.tourism.Constants
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.LoadImg
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.theme.TextStyles

@Composable
fun GalleryScreen(urls: List<String>, onItemClick: (String) -> Unit, onMoreClick: () -> Unit) {
    Column(Modifier.padding(Constants.SCREEN_PADDING)) {
        if (urls.isNotEmpty()) {
            LoadImg(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(200.dp)
                    .clickable {
                        onItemClick(urls.first())
                    }
                    .clip(imageShape),
                url = urls.first(),
            )
            VerticalSpace(height = 16.dp)

            Row {
                if (urls.size > 1) {
                    LoadImg(
                        modifier = Modifier
                            .weight(1f)
                            .clickable { onItemClick(urls[1]) }
                            .propertiesForSmallImage(),
                        url = urls[1],
                    )
                    if (urls.size > 2) {
                        HorizontalSpace(16.dp)
                        Box(
                            modifier = Modifier
                                .weight(1f)
                                .clickable { onMoreClick() }
                                .propertiesForSmallImage(),
                            contentAlignment = Alignment.Center
                        ) {
                            LoadImg(url = urls[2])
                            Box(
                                modifier = Modifier
                                    .fillMaxSize()
                                    .background(
                                        color = Color.Black.copy(alpha = 0.5f),
                                        shape = imageShape
                                    ),
                            )
                            Text(
                                text = "+${urls.size - 3}",
                                style = TextStyles.h1,
                                color = Color.White,
                            )
                        }
                    }
                }
            }
        }
    }
}
