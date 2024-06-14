package app.tourism.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import app.organicmaps.util.log.Logger
import com.skydoves.cloudy.Cloudy

@Composable
fun BlurryContainer(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    val localDensity = LocalDensity.current

    Box(Modifier.then(modifier)) {
        var height by remember { mutableStateOf(0.dp) }
        Cloudy(
            radius = 25,
            modifier = Modifier
                .fillMaxWidth()
                .height(height)
                .align(Alignment.Center)
                .clip(RoundedCornerShape(16.dp))
                .background(color = Color.White.copy(alpha = 0.25f))
        ) {}
        Column(
            Modifier
                .align(Alignment.Center)
                .onSizeChanged { newSize ->
                    height = with(localDensity) { newSize.height.toDp() }
                }
        ) {
            content()
        }
    }
}