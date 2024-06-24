package app.tourism.ui.screens.main.place_details

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import app.organicmaps.R
import app.tourism.data.dto.PlaceLocation
import app.tourism.ui.common.nav.AppTopBar

@Composable
fun PlaceDetailsScreen(
    id: Int,
    onBackClick: () -> Boolean,
    onMapClick: () -> Unit,
    onCreateRoute: (PlaceLocation) -> Unit,
) {
    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.profile_tourism),
                onBackClick = onBackClick,
            )
        }
    ) { paddingValues ->
        Column(Modifier.padding(paddingValues)) {
            // todo
            Text("id: $id")
        }
    }
}