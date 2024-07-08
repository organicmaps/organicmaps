package app.tourism.ui.common.nav

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import app.organicmaps.R
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getHintColor

@Composable
fun SearchTopBar(
    modifier: Modifier = Modifier,
    query: String,
    onQueryChanged: (String) -> Unit,
    onSearchClicked: ((String) -> Unit)? = null,
    onClearClicked: () -> Unit,
    onBackClicked: () -> Unit
) {
    val searchLabel = stringResource(id = R.string.search)

    TextField(
        modifier = Modifier
            .fillMaxWidth()
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
            IconButton(onClick = { onBackClicked() }) {
                Icon(
                    painter = painterResource(id = R.drawable.back),
                    contentDescription = stringResource(id = R.string.back),
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
        keyboardActions = KeyboardActions(onSearch = { onSearchClicked?.invoke(query) }),
        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Search),
        colors = TextFieldDefaults.colors(
            focusedContainerColor = MaterialTheme.colorScheme.background,
            unfocusedContainerColor = MaterialTheme.colorScheme.background,
        )
    )
}