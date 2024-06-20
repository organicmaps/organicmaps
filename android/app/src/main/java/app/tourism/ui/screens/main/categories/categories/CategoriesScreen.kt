package app.tourism.ui.screens.main.categories.categories

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import app.organicmaps.R
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData

@Composable
fun CategoriesScreen(
    onSiteClick: (id: Int) -> Unit,
    onMapClick: () -> Unit,
) {
    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.categories),
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