package app.tourism.ui.common

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
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
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import com.skydoves.cloudy.Cloudy
import com.skydoves.cloudy.CloudyState

@Composable
fun BlurryContainer(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    val localDensity = LocalDensity.current

    val cloudyState = remember { mutableStateOf<CloudyState>(CloudyState.Nothing) }

    Box(Modifier.then(modifier)) {
        var height by remember { mutableStateOf(0.dp) }
        Cloudy(
            radius = 25,
            modifier = Modifier
                .fillMaxWidth()
                .height(height)
                .align(Alignment.Center)
                .clip(RoundedCornerShape(16.dp)),

            onStateChanged = {
                println("cloudyState: $cloudyState")
                cloudyState.value = it
            }
        ) {}
        if (cloudyState.value is CloudyState.Success || cloudyState.value is CloudyState.Error || cloudyState.value is CloudyState.Loading)
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