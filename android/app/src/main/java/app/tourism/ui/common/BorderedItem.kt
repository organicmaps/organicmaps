package app.tourism.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getSelectedColor
import app.tourism.ui.theme.getSelectedTextColor

@Composable
fun BorderedItem(
    modifier: Modifier = Modifier,
    label: String,
    highlighted: Boolean = false,
    onClick: () -> Unit
) {
    val shape = RoundedCornerShape(16.dp)
    Text(
        modifier = Modifier
            .background(
                color = if (highlighted) getSelectedColor()
                else MaterialTheme.colorScheme.background,
                shape = shape
            )
            .clip(shape)
            .clickable {
                onClick()
            }
            .padding(12.dp)
            .then(modifier),
        text = label,
        color = getSelectedTextColor(),
        style = TextStyles.h4
    )
}