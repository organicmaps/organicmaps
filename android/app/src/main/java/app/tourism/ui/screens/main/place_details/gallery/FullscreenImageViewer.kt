import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTransformGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.PageSize
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import app.tourism.ui.common.nav.BackButton
import coil.compose.AsyncImage
import kotlinx.coroutines.launch

@Composable
fun FullscreenImageScreen(
    onBackClick: () -> Unit,
    selectedImageUrl: String,
    imageUrls: List<String>
) {
    val indexOfSelectedImage = imageUrls.indexOfFirst { it == selectedImageUrl }
    val pagerState =
        rememberPagerState(initialPage = if (indexOfSelectedImage != -1) indexOfSelectedImage else 0) { imageUrls.size }
    val scope = rememberCoroutineScope()

    Box(modifier = Modifier.fillMaxSize()) {


        HorizontalPager(
            pageSize = PageSize.Fill,
            state = pagerState,
            modifier = Modifier.fillMaxSize()
        ) { page ->
            var scale by remember { mutableStateOf(1f) }
            var offset by remember { mutableStateOf(Offset.Zero) }
            var zooming by remember { mutableStateOf(false) }

            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .pointerInput(Unit) {
                        detectTransformGestures { centroid, pan, zoom, _ ->
                            if (zoom != 1f) zooming = true
                            scale = (scale * zoom).coerceIn(1f, 3f)
                            if (scale > 1f) {
                                val newOffset = offset + pan
                                val maxX = size.width * (scale - 1) / 2
                                val maxY = size.height * (scale - 1) / 2
                                offset = Offset(
                                    newOffset.x.coerceIn(-maxX, maxX),
                                    newOffset.y.coerceIn(-maxY, maxY)
                                )
                            } else {
                                offset = Offset.Zero
                                if (!zooming) {
                                    // Allow swiping when not zoomed
                                    scope.launch {
                                        if (pan.x > 0 && pagerState.currentPage > 0) {
                                            pagerState.animateScrollToPage(pagerState.currentPage - 1)
                                        } else if (pan.x < 0 && pagerState.currentPage < imageUrls.size - 1) {
                                            pagerState.animateScrollToPage(pagerState.currentPage + 1)
                                        }
                                    }
                                }
                            }
                            zooming = false
                        }
                    }
            ) {
                AsyncImage(
                    model = imageUrls[page],
                    contentDescription = "Full screen image",
                    contentScale = ContentScale.FillWidth,
                    modifier = Modifier
                        .fillMaxSize()
                        .graphicsLayer(
                            scaleX = scale,
                            scaleY = scale,
                            translationX = offset.x,
                            translationY = offset.y
                        )
                )
            }
        }
        Box(Modifier.padding(16.dp)) {
            BackButton(
                size = 30.dp,
                onBackClick = onBackClick,
            )
        }
        // Page indicator
        Row(
            Modifier
                .height(50.dp)
                .fillMaxWidth()
                .align(Alignment.BottomCenter),
            horizontalArrangement = Arrangement.Center
        ) {
            repeat(imageUrls.size) { iteration ->
                val color =
                    if (pagerState.currentPage == iteration) MaterialTheme.colorScheme.primary
                    else MaterialTheme.colorScheme.primary.copy(alpha = 0.25f)
                Box(
                    modifier = Modifier
                        .padding(2.dp)
                        .clip(CircleShape)
                        .background(color)
                        .size(8.dp)
                )
            }
        }
    }
}