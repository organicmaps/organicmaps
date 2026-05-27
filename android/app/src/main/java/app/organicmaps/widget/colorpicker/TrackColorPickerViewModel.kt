package app.organicmaps.widget.colorpicker

import android.app.Application
import android.content.Context
import androidx.annotation.ColorInt
import androidx.core.content.edit
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

enum class ColorPickerTab { GRID, SPECTRUM, SLIDERS }

data class ColorPickerState(
    @ColorInt val currentColor: Int,
    val presetColors: List<Int>,
    val selectedPresetIndex: Int,
    val activeTab: ColorPickerTab = ColorPickerTab.SPECTRUM,
    val colorChanged: Boolean = false,
)

class TrackColorPickerViewModel(application: Application, private val savedStateHandle: SavedStateHandle) :
    AndroidViewModel(application) {

    @ColorInt
    val initialColor = savedStateHandle[EXTRA_INITIAL_COLOR] ?: DEFAULT_PRESETS[0]

    private val restoredTab = savedStateHandle.get<String>(KEY_ACTIVE_TAB)
        ?.let { name -> ColorPickerTab.entries.firstOrNull { it.name == name } }
        ?: ColorPickerTab.SPECTRUM

    private val _state = MutableStateFlow(
        ColorPickerState(
            currentColor = initialColor,
            presetColors = emptyList(),
            selectedPresetIndex = -1,
            activeTab = restoredTab,
        ),
    )
    val state = _state.asStateFlow()

    @OptIn(ExperimentalCoroutinesApi::class)
    private val saveDispatcher = Dispatchers.IO.limitedParallelism(1)

    init {
        viewModelScope.launch {
            val presets = withContext(Dispatchers.IO) { loadPresets() }
            _state.update {
                it.copy(
                    presetColors = presets,
                    selectedPresetIndex = presets.indexOf(initialColor),
                )
            }
        }

        viewModelScope.launch {
            _state.collect { state ->
                savedStateHandle[KEY_ACTIVE_TAB] = state.activeTab.name
            }
        }
    }

    @ColorInt
    fun getResultColor(): Int? {
        val state = _state.value
        return if (state.colorChanged) state.currentColor else null
    }

    fun setActiveTab(tab: ColorPickerTab) {
        _state.update { it.copy(activeTab = tab) }
    }

    fun selectColor(@ColorInt color: Int) {
        _state.update {
            it.copy(
                currentColor = color,
                selectedPresetIndex = it.presetColors.indexOf(color),
                colorChanged = true,
            )
        }
    }

    fun selectPreset(index: Int) {
        _state.update { state ->
            if (index !in state.presetColors.indices) {
                state
            } else {
                state.copy(
                    currentColor = state.presetColors[index],
                    selectedPresetIndex = index,
                    colorChanged = true,
                )
            }
        }
    }

    fun canAddPreset(): Boolean {
        val state = _state.value
        return state.presetColors.size < MAX_PRESETS && state.currentColor !in state.presetColors
    }

    fun addCurrentColorToPresets() {
        var presetsToSave: List<Int>? = null
        _state.update { state ->
            if (state.presetColors.size >= MAX_PRESETS || state.currentColor in state.presetColors) {
                state
            } else {
                val updated = state.presetColors + state.currentColor
                presetsToSave = updated
                state.copy(
                    presetColors = updated,
                    selectedPresetIndex = updated.lastIndex,
                )
            }
        }
        presetsToSave?.let { savePresets(it) }
    }

    fun removePreset(index: Int) {
        var presetsToSave: List<Int>? = null
        _state.update { state ->
            if (index !in state.presetColors.indices) {
                state
            } else {
                val updated = state.presetColors.toMutableList().apply { removeAt(index) }
                presetsToSave = updated
                val newIndex = when {
                    state.selectedPresetIndex == index -> -1
                    state.selectedPresetIndex > index -> state.selectedPresetIndex - 1
                    else -> state.selectedPresetIndex
                }
                state.copy(
                    presetColors = updated,
                    selectedPresetIndex = newIndex,
                )
            }
        }
        presetsToSave?.let { savePresets(it) }
    }

    private fun loadPresets(): List<Int> {
        val saved = getApplication<Application>()
            .getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getString(KEY_COLORS, null)
        if (!saved.isNullOrEmpty()) {
            val colors = saved.split(",").mapNotNull { it.toLongOrNull(16)?.toInt() }
            if (colors.isNotEmpty()) return colors
        }
        return DEFAULT_PRESETS.toList()
    }

    private fun savePresets(colors: List<Int>) {
        viewModelScope.launch(saveDispatcher) {
            getApplication<Application>()
                .getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                .edit { putString(KEY_COLORS, colors.joinToString(",") { "%08X".format(it) }) }
        }
    }

    companion object {
        const val EXTRA_INITIAL_COLOR = "ExtraInitialColor"
        private const val KEY_ACTIVE_TAB = "active_tab"
        private const val MAX_PRESETS = 20
        private const val PREFS_NAME = "track_color_presets"
        private const val KEY_COLORS = "saved_colors"

        @ColorInt
        private val DEFAULT_PRESETS = intArrayOf(
            0xFF000000.toInt(), // Black
            0xFF0066CC.toInt(), // Blue
            0xFF3C8C3C.toInt(), // Green
            0xFFFFC800.toInt(), // Yellow
            0xFFE51B23.toInt(), // Red
        )
    }
}
