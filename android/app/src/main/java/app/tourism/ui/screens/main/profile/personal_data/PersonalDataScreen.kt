package app.tourism.ui.screens.main.profile.personal_data

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import app.organicmaps.R
import app.tourism.ui.common.nav.AppTopBar

@Composable
fun PersonalDataScreen(onBackClick: () -> Boolean) {
    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.personal_data),
                onBackClick = onBackClick,
            )
        }
    ) { paddingValues ->
        Column(Modifier.padding(paddingValues)) {
            // todo
        }
    }
}