package app.tourism.ui.common.ui_state

import androidx.compose.animation.*
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier

@Composable
fun Loading(modifier: Modifier = Modifier, status: Boolean = true, onEntireScreen: Boolean = true) {
    AnimatedVisibility(
        visible = status,
        enter = fadeIn(),
        exit = fadeOut()
    ) {
        Box(
            modifier = if (onEntireScreen) modifier.fillMaxSize() else modifier.fillMaxWidth(),
            contentAlignment = Alignment.Center
        ) {
            CircularProgressIndicator(color = MaterialTheme.colorScheme.primary)
        }
    }
}