package app.tourism.ui.screens.main

import androidx.lifecycle.ViewModel
import app.tourism.data.prefs.UserPreferences
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import javax.inject.Inject

@HiltViewModel
class ThemeViewModel @Inject constructor(
    private val userPreferences: UserPreferences,
) : ViewModel() {
    private val _theme = MutableStateFlow(userPreferences.getTheme())
    val theme = _theme.asStateFlow()

    fun setTheme(themeCode: String) {
        _theme.value = userPreferences.themes.first { it.code == themeCode }
        userPreferences.setTheme(themeCode)
    }
}