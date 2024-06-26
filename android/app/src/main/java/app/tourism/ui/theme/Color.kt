package app.tourism.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

val Blue = Color(0xFF0688E7)
val LightBlue = Color(0xFF3FAAF8)
val LighterBlue = Color(0xFFCADCF9)
val LightestBlue = Color(0xFFEEF4FF)
val DarkerBlue = Color(0xFF272f46)
val DarkestBlue = Color(0xFF101832)
val HeartRed = Color(0xFFFF6C61)
val StarYellow = Color(0xFFF8D749)
val DarkGrayForText = Color(0xFF78787F)
val Gray = Color(0xFFD5D5D6)
val LightGray = Color(0xFFF4F4F4)
val DarkForText = Color(0xFF2B2D33)
val WhiteForText = Color(0xFFFFFFFF)

val BorderDay = Color(0xFFC9D4E7)
val BorderNight = Color(0xFFFFFFFF)
@Composable
fun getBorderColor() = if (isSystemInDarkTheme()) BorderNight else BorderDay

val HintDay = Color(0xFFAAABAD)
val HintNight = Color(0xFFAAABAD)
@Composable
fun getHintColor() = if (isSystemInDarkTheme()) HintNight else HintDay

@Composable
fun getStarColor() = StarYellow