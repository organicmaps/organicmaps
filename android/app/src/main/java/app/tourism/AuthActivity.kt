package app.tourism

import android.content.Context
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.ui.Modifier
import androidx.lifecycle.lifecycleScope
import app.organicmaps.R
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.PlacesRepository
import app.tourism.ui.screens.auth.AuthNavigation
import app.tourism.ui.theme.OrganicMapsTheme
import app.tourism.utils.LocaleHelper
import app.tourism.utils.MapFileMover
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.launch
import javax.inject.Inject

@AndroidEntryPoint
class AuthActivity : ComponentActivity() {
    @Inject
    lateinit var placesRepository: PlacesRepository

    @Inject
    lateinit var userPreferences: UserPreferences

    override fun attachBaseContext(newBase: Context) {
        val languageCode = UserPreferences(newBase).getLanguage()?.code
        super.attachBaseContext(LocaleHelper.localeUpdateResources(newBase, languageCode ?: "ru"))
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        lifecycleScope.launch {
            if (!userPreferences.getIsEverythingSetup()) {
                if (!MapFileMover().main(this@AuthActivity)) return@launch
                userPreferences.setIsEverythingSetup(true)
            }
        }

        val blackest = resources.getColor(R.color.button_text) // yes, I know
        enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.dark(blackest),
            navigationBarStyle = SystemBarStyle.dark(blackest)
        )
        setContent {
            OrganicMapsTheme() {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Column(modifier = Modifier.padding(innerPadding)) {
                        AuthNavigation()
                    }
                }
            }
        }
    }
}