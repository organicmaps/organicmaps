package app.tourism.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val lightColors = lightColorScheme(
    primary = Blue,
    onPrimary = Color.White,
    surface = LightestBlue,
    onSurface = DarkForText,
    background = Color.White,
    onBackground = DarkForText,
    error = Color.Transparent,
    onError = Color.Red,
)

private val darkColors = darkColorScheme(
    primary = Blue,
    onPrimary = Color.White,
    surface = DarkerBlue,
    onSurface = WhiteForText,
    background = DarkestBlue,
    onBackground = WhiteForText,
    error = Color.Transparent,
    onError = Color.Red,
)


@Composable
fun OrganicMapsTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    // Dynamic color is available on Android 12+
    dynamicColor: Boolean = true,
    content: @Composable () -> Unit
) {
    val colorScheme = when {
        darkTheme -> darkColors
        else -> lightColors
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = Typography,
        content = content
    )
}