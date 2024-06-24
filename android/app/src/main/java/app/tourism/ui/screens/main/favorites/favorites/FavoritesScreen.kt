package app.tourism.ui.screens.main.favorites.favorites

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import app.organicmaps.R
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData

@Composable
fun FavoritesScreen(
    onPlaceClick: (id: Int) -> Unit,
) {
    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.favorites),
                actions = listOf(
                    TopBarActionData(
                        iconDrawable = R.drawable.search,
                        onClick = {
                            // todo
                        }
                    ),
                ),
            )
        }
    ) { paddingValues ->
        Column(Modifier.padding(paddingValues)) {
            // todo

        }
    }
}