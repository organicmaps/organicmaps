package app.tourism.ui.common

import android.webkit.WebView
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.viewinterop.AndroidView
import androidx.webkit.WebSettingsCompat
import androidx.webkit.WebViewFeature

@Composable
fun WebView(data: String) {
    AndroidView(
        factory = { context ->
            WebView(context).apply {
                settings.loadWithOverviewMode = true
                loadData(data, "text/html", "UTF-8")
            }
        },
        update = {
            it.loadData(data, "text/html", "UTF-8")
        }
    )
}