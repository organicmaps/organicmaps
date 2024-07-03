package app.tourism

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.runtime.collectAsState
import androidx.core.content.ContextCompat.startActivity
import androidx.lifecycle.lifecycleScope
import app.organicmaps.DownloadResourcesLegacyActivity
import app.organicmaps.R
import app.organicmaps.downloader.CountryItem
import app.tourism.data.prefs.UserPreferences
import app.tourism.domain.models.resource.Resource
import app.tourism.ui.screens.main.MainSection
import app.tourism.ui.screens.main.ThemeViewModel
import app.tourism.ui.screens.main.profile.profile.ProfileViewModel
import app.tourism.ui.theme.OrganicMapsTheme
import app.tourism.utils.changeSystemAppLanguage
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import javax.inject.Inject

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    @Inject
    lateinit var userPreferences: UserPreferences

    private val themeVM: ThemeViewModel by viewModels<ThemeViewModel>()
    private val profileVM: ProfileViewModel by viewModels<ProfileViewModel>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        navigateToMapToDownloadIfNotPresent()
        navigateToAuthIfNotAuthed()

        val blackest = resources.getColor(R.color.button_text) // yes, I know
        enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.dark(blackest),
            navigationBarStyle = SystemBarStyle.dark(blackest)
        )

        setContent {
            val isDark = themeVM.theme.collectAsState().value?.code == "dark"

            OrganicMapsTheme(darkTheme = isDark) {
                MainSection(themeVM)
            }
        }
    }

    private fun navigateToMapToDownloadIfNotPresent() {
        val mCurrentCountry = CountryItem.fill("Tajikistan")
        if (!mCurrentCountry.present) {
            val intent = Intent(this, DownloadResourcesLegacyActivity::class.java)
            startActivity(this, intent, null)
        }
    }

    private fun navigateToAuthIfNotAuthed() {
        val token = userPreferences.getToken()
        if (token.isNullOrEmpty()) navigateToAuth()

        profileVM.getPersonalData()
        lifecycleScope.launch {
            profileVM.profileDataResource.collectLatest {
                if (it is Resource.Success) {
                    it.data?.language?.let { lang ->
                        changeSystemAppLanguage(this@MainActivity, lang)
                        userPreferences.setLanguage(lang)
                    }
                    it.data?.theme?.let { theme ->
                        themeVM.setTheme(theme)
                    }
                }
                if (it is Resource.Error) {
                    if (it.message?.contains("unauth", ignoreCase = true) == true)
                        navigateToAuth()
                }
            }
        }
    }

    private fun navigateToAuth() {
        val intent = Intent(this, AuthActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK or Intent.FLAG_ACTIVITY_NEW_TASK)
        startActivity(this, intent, null)
    }
}
