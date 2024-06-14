package app.tourism.ui.common.nav

import androidx.annotation.DrawableRes
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import app.tourism.Constants
import app.tourism.ui.theme.TextStyles

@Composable
fun AppTopBar(
    modifier: Modifier = Modifier,
    title: String,
    onBackClick: (() -> Boolean)? = null,
    actions: List<TopBarActionData> = emptyList()
) {
    Column(modifier = Modifier.then(modifier)) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            onBackClick?.let { BackButton(onBackClick = onBackClick) }
                Row {
                    actions.forEach {
                        TopBarAction(iconDrawable = it.iconDrawable, onClick = it.onClick)
                    }
                }

        }

        Column(Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
            Text(text = title, style = TextStyles.h1, color = MaterialTheme.colorScheme.onBackground)
        }
    }
}

data class TopBarActionData(@DrawableRes val iconDrawable: Int, val onClick: () -> Unit)

@Composable
fun TopBarAction(@DrawableRes iconDrawable: Int, onClick: () -> Unit) {
    IconButton(onClick = onClick) {
        Icon(
            modifier = Modifier.size(24.dp),
            painter = painterResource(id = iconDrawable),
            contentDescription = null,
        )
    }
}
