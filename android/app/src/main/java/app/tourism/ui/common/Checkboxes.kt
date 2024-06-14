package app.tourism.ui.common

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.theme.TextStyles

@Composable
fun SingleChoiceCheckBoxes(
    modifier: Modifier = Modifier,
    selectedItemName: String?,
    itemNames: List<String>,
    onItemChecked: (String) -> Unit
) {
    Column(Modifier.then(modifier)) {
        itemNames.forEach { name ->
            CheckBoxItem(
                name = name,
                checked = if(selectedItemName != null) selectedItemName == name else false,
                onItemChecked = { onItemChecked(name) },
            )
        }
    }
}

@Composable
fun CheckBoxItem(
    modifier: Modifier = Modifier,
    name: String,
    checked: Boolean,
    onItemChecked: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { onItemChecked() }
            .padding(16.dp)
            .then(modifier),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = name,
            style = TextStyles.h4,
            color = MaterialTheme.colorScheme.onBackground
        )
        Icon(
            painter = painterResource(id = if (checked) R.drawable.check_circle_fill else R.drawable.unchecked),
            tint = MaterialTheme.colorScheme.primary,
            contentDescription = null,
        )
    }
}
