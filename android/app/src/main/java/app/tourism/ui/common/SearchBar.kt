package app.tourism.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import app.organicmaps.R
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getHintColor

@Composable
fun AppSearchBar(
    modifier: Modifier = Modifier,
    query: String,
    onQueryChanged: (String) -> Unit,
    onSearchClicked: (String) -> Unit,
    onClearClicked: () -> Unit,
) {
    var isActive by remember { mutableStateOf(false) }

    val searchLabel = stringResource(id = R.string.search)

    OutlinedTextField(
        modifier = Modifier
            .clickable { isActive = true }
            .then(modifier),
        value = query,
        onValueChange = onQueryChanged,
        placeholder = {
            Text(
                text = searchLabel,
                style = TextStyles.h4.copy(color = getHintColor()),
            )
        },
        singleLine = true,
        maxLines = 1,
        leadingIcon = {
            IconButton(onClick = { onSearchClicked(query) }) {
                Icon(
                    painter = painterResource(id = R.drawable.search),
                    contentDescription = searchLabel,
                    tint = getHintColor()
                )
            }
        },
        trailingIcon = {
            if (query.isNotEmpty())
                IconButton(onClick = { onClearClicked() }) {
                    Icon(
                        painter = painterResource(id = R.drawable.ic_clear_rounded),
                        contentDescription = stringResource(id = R.string.clear_search_field),
                    )
                }
        },
        shape = RoundedCornerShape(16.dp),
        keyboardActions = KeyboardActions(onSearch = { onSearchClicked(query) }),
        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Search),
        colors = TextFieldDefaults.colors(
            focusedContainerColor = MaterialTheme.colorScheme.surface,
            unfocusedContainerColor = MaterialTheme.colorScheme.surface,
            unfocusedIndicatorColor = MaterialTheme.colorScheme.surface
        ),
    )
}