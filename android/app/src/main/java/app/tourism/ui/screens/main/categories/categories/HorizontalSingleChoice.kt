package app.tourism.ui.screens.main.categories.categories

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import app.tourism.applyAppBorder
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.models.SingleChoiceItem
import app.tourism.ui.theme.TextStyles

@Composable
fun HorizontalSingleChoice(
    modifier: Modifier = Modifier,
    items: List<SingleChoiceItem>,
    selected: SingleChoiceItem?,
    onSelectedChanged: (SingleChoiceItem) -> Unit
) {
    Row(Modifier.then(modifier)) {
        items.forEach {
            SingleChoiceItem(
                item = it,
                isSelected = it.key == selected?.key,
                onClick = {
                    onSelectedChanged(it)
                },
            )
            HorizontalSpace(width = 12.dp)
        }
    }
}

@Composable
private fun SingleChoiceItem(
    modifier: Modifier = Modifier,
    item: SingleChoiceItem,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    val shape = RoundedCornerShape(16.dp)
    Text(
        modifier = Modifier
            .applyAppBorder()
            .clickable {
                onClick()
            }
            .clip(shape)
            .background(
                color = if (isSelected) MaterialTheme.colorScheme.surface
                else MaterialTheme.colorScheme.background,
                shape = shape
            )
            .padding(12.dp)
            .then(modifier),
        text = item.label,
        style = TextStyles.h4
    )
}
