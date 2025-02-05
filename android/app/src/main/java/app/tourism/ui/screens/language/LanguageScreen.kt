package app.tourism.ui.screens.language

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.organicmaps.SplashActivity
import app.tourism.ui.common.SingleChoiceCheckBoxes
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.utils.changeSystemAppLanguage
import app.tourism.utils.triggerRebirth

@Composable
fun LanguageScreen(
    onBackClick: () -> Unit,
    vm: LanguageViewModel = hiltViewModel()
) {
    val context = LocalContext.current
    val languages by vm.languages.collectAsState()
    val selectedLanguage by vm.selectedLanguage.collectAsState()
    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.chose_language),
                onBackClick = {
                    onBackClick()
                }
            )
        },
        containerColor = MaterialTheme.colorScheme.background,
    ) { paddingValues ->
        Column(Modifier.padding(paddingValues)) {
            // todo
            VerticalSpace(height = 16.dp)
            SingleChoiceCheckBoxes(
                itemNames = languages.map { it.name },
                selectedItemName = if (selectedLanguage != null) selectedLanguage?.name else null,
                onItemChecked = { name ->
                    val language = languages.first { it.name == name }
                    vm.updateLanguage(language)
                    changeSystemAppLanguage(context, language.code)
                    triggerRebirth(context, SplashActivity::class.java)
                }
            )
        }
    }
}

