package app.organicmaps.util

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material.Colors
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Shapes
import androidx.compose.material.Typography
import androidx.compose.material.darkColors
import androidx.compose.material.lightColors
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalInspectionMode
import com.google.accompanist.systemuicontroller.rememberSystemUiController

// **********************************
// Overwrite material theme colors
// **********************************
private val LightColors = lightColors(
    primary = color_primary,
    onPrimary = color_on_primary,
    secondary = color_secondary,
    onSecondary = color_on_secondary,
)
private val DarkColors = darkColors(
    primary = color_primary_dark,
    onPrimary = color_on_primary_dark,
    secondary = color_secondary_dark,
    onSecondary = color_on_secondary_dark,
)

// **********************************
// Define new theme colors
// **********************************

// Defines a new class to hold material colors along with new ones
// This is only to define the type, for the values see below
@Immutable
data class ExtendedColors(
    val material: Colors = LightColors,
    val iconTint: Color = Color.Unspecified,
    val colorLogo: Color = Color.Unspecified,
    val statusBar: Color = Color.Unspecified,
)

// Define the value for the new colors
private val LightExtendedColors = ExtendedColors(
    material = LightColors,
    iconTint = color_icon_tint,
    colorLogo = color_logo,
    statusBar = color_status_bar
)
private val DarkExtendedColors = ExtendedColors(
    material = DarkColors,
    iconTint = color_icon_tint_dark,
    colorLogo = color_logo_dark,
    statusBar = color_status_bar_dark
)

val LocalExtendedColors = staticCompositionLocalOf {
    ExtendedColors()
}

// Create a new theme object extending the material theme with our own colors
// We can use this object to access our custom colors or default Material properties
object ExtendedMaterialTheme {
    val colors: ExtendedColors
        @Composable
        get() = LocalExtendedColors.current
    val shapes: Shapes
        @Composable
        get() = MaterialTheme.shapes
    val typography: Typography
        @Composable
        get() = MaterialTheme.typography
}


// **********************************
// Create the actual theme composable
// **********************************
@Composable
fun Theme(
    content: @Composable () -> Unit
) {
    // Do not use ThemeUtils if the component is running in a preview window
    val nightMode = if (LocalInspectionMode.current)
        isSystemInDarkTheme()
    else
        ThemeUtils.isNightTheme(LocalContext.current)

    val systemUiController = rememberSystemUiController()
    // Return our extended theme for use in our custom components
    CompositionLocalProvider(LocalExtendedColors provides if (nightMode) DarkExtendedColors else LightExtendedColors) {
        val statusBarColor = ExtendedMaterialTheme.colors.statusBar
        DisposableEffect(systemUiController, statusBarColor) {
            systemUiController.setStatusBarColor(
                color = statusBarColor,
                darkIcons = false
            )
            onDispose {}
        }
        // Return MaterialTheme so base components can read our overwritten properties
        MaterialTheme(
            colors = LocalExtendedColors.current.material,
            content = content
        )
    }
}