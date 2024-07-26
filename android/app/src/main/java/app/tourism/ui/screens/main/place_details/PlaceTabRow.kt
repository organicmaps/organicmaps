package app.tourism.ui.screens.main.place_details

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.models.SingleChoiceItem
import app.tourism.ui.theme.DarkestBlue
import app.tourism.ui.theme.SelectedDay
import app.tourism.ui.theme.SelectedNight
import app.tourism.ui.theme.SelectedTextDay
import app.tourism.ui.theme.SelectedTextNight
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getSelectedColor
import app.tourism.ui.theme.getSelectedTextColor

@Composable
fun PlaceTabRow(modifier: Modifier = Modifier, tabIndex: Int, onTabIndexChanged: (Int) -> Unit) {
    val tabs = listOf(
        SingleChoiceItem(0, stringResource(id = R.string.description_tourism)),
        SingleChoiceItem(1, stringResource(id = R.string.gallery)),
        SingleChoiceItem(2, stringResource(id = R.string.reviews)),
    )

    val shape = RoundedCornerShape(50.dp)

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                color = MaterialTheme.colorScheme.surface,
                shape
            )
            .then(modifier)
            .padding(8.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
    ) {
        tabs.forEach {
            SingleChoiceItem(
                item = it,
                isSelected = it.key == tabIndex,
                onClick = {
                    onTabIndexChanged(it.key as Int)
                },
            )
        }
    }
}

@Composable
private fun SingleChoiceItem(
    item: SingleChoiceItem,
    isSelected: Boolean,
    onClick: () -> Unit,
) {
    val shape = RoundedCornerShape(50.dp)
    val notSelectedColor = MaterialTheme.colorScheme.surface
    val notSelectedTextColor = MaterialTheme.colorScheme.onBackground
    val selectedColor = getSelectedColor()
    val selectedTextColor = getSelectedTextColor()

    Text(
        modifier = Modifier
            .wrapContentSize()
            .clip(shape)
            .clickable { onClick() }
            .background(
                color = if (isSelected) selectedColor else notSelectedColor,
                shape = shape
            )
            .padding(8.dp),
        text = item.label,
        style = TextStyles.b1,
        color = if (isSelected) selectedTextColor else notSelectedTextColor,
        maxLines = 1
    )
}
