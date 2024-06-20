package app.tourism.ui.common.nav

import androidx.annotation.DrawableRes
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.theme.TextStyles

@Composable
fun AppTopBar(
    modifier: Modifier = Modifier,
    title: String,
    onBackClick: (() -> Boolean)? = null,
    actions: List<TopBarActionData> = emptyList()
) {
    Column(
        Modifier
            .padding(horizontal = 16.dp, vertical = 12.dp)
            .then(modifier)
    ) {
        Box(Modifier.fillMaxWidth()) {
            onBackClick?.let {
                BackButton(
                    modifier.align(Alignment.CenterStart),
                    onBackClick = onBackClick
                )
            }
            Row(modifier.align(Alignment.CenterEnd)) {
                actions.forEach {
                    TopBarAction(
                        iconDrawable = it.iconDrawable,
                        color = it.color,
                        onClick = it.onClick
                    )
                }
            }
        }
        VerticalSpace(height = 12.dp)

        Text(
            text = title,
            style = TextStyles.h1,
            color = MaterialTheme.colorScheme.onBackground
        )
    }
}

data class TopBarActionData(
    @DrawableRes val iconDrawable: Int,
    val color: Color? = null,
    val onClick: () -> Unit
)

@Composable
fun TopBarAction(
    @DrawableRes iconDrawable: Int,
    color: Color? = null,
    onClick: () -> Unit,
) {
    IconButton(onClick = onClick) {
        Icon(
            modifier = Modifier
                .size(30.dp),
            painter = painterResource(id = iconDrawable),
            tint = color ?: MaterialTheme.colorScheme.onBackground,
            contentDescription = null,
        )
    }
}
