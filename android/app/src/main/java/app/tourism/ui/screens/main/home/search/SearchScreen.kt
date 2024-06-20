package app.tourism.ui.screens.main.home.search

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import app.organicmaps.R
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData

@Composable
fun SearchScreen(
    onSiteClick: (id: Int) -> Unit,
    onMapClick: () -> Unit,
) {
    Scaffold(
        topBar = {
            AppTopBar(
                // todo remove hardcoded value
                title = "Search",
                actions = listOf(
                    TopBarActionData(
                        iconDrawable = R.drawable.map,
                        color = MaterialTheme.colorScheme.primary,
                        onClick = onMapClick
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