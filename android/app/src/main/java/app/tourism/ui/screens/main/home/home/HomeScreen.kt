package app.tourism.ui.screens.main.home.home

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.SpaceForNavBar
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData

@Composable
fun HomeScreen(
    onSearchClick: (String) -> Unit,
    onSiteClick: (id: Int) -> Unit,
    onMapClick: () -> Unit,
) {
    Scaffold(
        topBar = {
            AppTopBar(
                // todo remove hardcoded value
                title = "Душанбе",
                actions = listOf(
                    TopBarActionData(
                        iconDrawable = R.drawable.map,
                        color = MaterialTheme.colorScheme.primary,
                        onClick = onMapClick
                    ),
                ),
            )
        },
        contentWindowInsets = Constants.USUAL_WINDOW_INSETS
    ) { paddingValues ->
        Column(
            Modifier
                .padding(paddingValues)
                .verticalScroll(rememberScrollState())
        ) {
            // todo
            PrimaryButton(label = "navigate to Site details screen", onClick = { onSiteClick(1) })

            repeat(50) {
                Text(text = "sldkjfsdlkf")
            }
            SpaceForNavBar()
        }
    }
}
