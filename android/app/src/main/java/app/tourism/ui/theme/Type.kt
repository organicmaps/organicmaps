package app.tourism.ui.theme

import androidx.compose.material3.Typography
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import app.organicmaps.R

object Fonts {
    val gilroy_regular = FontFamily(
        Font(R.font.gilroy_regular)
    )
}


object TextStyles {
    private val genericStyle = TextStyle(
        fontFamily = Fonts.gilroy_regular,
        fontWeight = FontWeight.Normal,
    )

    val humongous = genericStyle.copy(
        fontWeight = FontWeight.ExtraBold,
        fontSize = 36.sp,
        lineHeight = 40.sp,
    )

    val h1 = genericStyle.copy(
        fontWeight = FontWeight.SemiBold,
        fontSize = 32.sp,
        lineHeight = 36.sp,
    )

    val h2 = genericStyle.copy(
        fontWeight = FontWeight.SemiBold,
        fontSize = 24.sp,
        lineHeight = 36.sp,
    )

    val h3 = genericStyle.copy(
        fontSize = 20.sp,
        lineHeight = 22.sp,
        fontWeight = FontWeight.SemiBold,
    )

    val h4 = genericStyle.copy(
        fontSize = 16.sp,
        fontWeight = FontWeight.Medium,
    )

    val b1 = genericStyle.copy(
        fontSize = 14.sp
    )

    val b2 = genericStyle.copy(
        fontSize = 12.sp
    )

    val b3 = genericStyle.copy(
        fontSize = 10.sp
    )
}

// Set of Material typography styles to start with
val Typography = Typography(
    bodyLarge = TextStyle(
        fontFamily = FontFamily.Default,
        fontWeight = FontWeight.Normal,
        fontSize = 16.sp,
        lineHeight = 24.sp,
        letterSpacing = 0.5.sp
    )
    /* Other default text styles to override
    titleLarge = TextStyle(
        fontFamily = FontFamily.Default,
        fontWeight = FontWeight.Normal,
        fontSize = 22.sp,
        lineHeight = 28.sp,
        letterSpacing = 0.sp
    ),
    labelSmall = TextStyle(
        fontFamily = FontFamily.Default,
        fontWeight = FontWeight.Medium,
        fontSize = 11.sp,
        lineHeight = 16.sp,
        letterSpacing = 0.5.sp
    )
    */
)