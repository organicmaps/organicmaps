package app.organicmaps.settings

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.widget.SwitchCompat
import androidx.core.view.ViewCompat
import androidx.core.widget.doAfterTextChanged
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener
import com.google.android.material.slider.Slider
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout

// Settings sub-screen for the custom raster background tiles (XYZ {z}/{x}/{y} source).
// Values are persisted in the C++ core (not SharedPreferences); the layer is rendered when the
// "enabled" switch is on. Edits are applied together in onPause() when the user leaves the screen.
class BgTilesSettingsFragment : BaseMwmFragment() {

    private lateinit var enabledSwitch: SwitchCompat
    private lateinit var urlInputLayout: TextInputLayout
    private lateinit var urlField: TextInputEditText
    private lateinit var cacheSizeSlider: Slider
    private lateinit var cacheSizeLabel: TextView
    private lateinit var opacitySlider: Slider
    private lateinit var opacityLabel: TextView

    private val currentUrl: String
        get() = urlField.text.toString().trim()

    // The URL is validated only when the layer is enabled (a disabled layer is never rendered, so the
    // URL may stay empty/unset). Closing is always allowed: an invalid config simply isn't applied
    // (see onPause), and the field is highlighted in red as feedback.
    private val isConfigValid: Boolean
        get() = !enabledSwitch.isChecked || Framework.nativeIsWellFormedBackgroundTilesUrl(currentUrl)

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_bg_tiles_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(view, ScrollableContentInsetsListener(view))

        enabledSwitch = view.findViewById(R.id.bg_tiles_enabled)
        urlInputLayout = view.findViewById(R.id.bg_tiles_url_input)
        urlField = view.findViewById(R.id.bg_tiles_url)
        cacheSizeSlider = view.findViewById(R.id.bg_tiles_size)
        cacheSizeLabel = view.findViewById(R.id.bg_tiles_size_value)
        opacitySlider = view.findViewById(R.id.bg_tiles_opacity)
        opacityLabel = view.findViewById(R.id.bg_tiles_opacity_value)

        // Initial values come from the core; on configuration change Android's view-state restore
        // overlays the in-flight UI values on top of these, which is the intended behavior.
        val isEnabled = Framework.nativeIsBackgroundTilesEnabled()
        enabledSwitch.isChecked = isEnabled
        urlField.setText(Framework.nativeGetBackgroundTilesUrl())
        // Clamp to the slider range to be safe at the JNI boundary: Slider.setValue throws if the
        // value is outside [valueFrom, valueTo].
        val cacheSizeMb = Framework.nativeGetBackgroundTilesCacheSizeMB()
        cacheSizeSlider.value = cacheSizeMb.toFloat().coerceIn(cacheSizeSlider.valueFrom, cacheSizeSlider.valueTo)
        cacheSizeLabel.text = "${cacheSizeSlider.value.toInt()}"
        val opacityPct = Framework.nativeGetBackgroundTilesAreaOpacity()
        opacitySlider.value = opacityPct.toFloat().coerceIn(opacitySlider.valueFrom, opacitySlider.valueTo)
        opacityLabel.text = "${opacitySlider.value.toInt()}"

        updateFieldsEnabled(isEnabled)

        enabledSwitch.setOnCheckedChangeListener { _, isChecked ->
            updateFieldsEnabled(isChecked)
            refreshValidation()
        }
        urlField.doAfterTextChanged { refreshValidation() }
        cacheSizeSlider.bindValueLabel(cacheSizeLabel)
        opacitySlider.bindValueLabel(opacityLabel)

        refreshValidation()
    }

    override fun onPause() {
        // Don't apply (and persist) a malformed config: just leave the previously saved settings intact.
        if (isConfigValid) {
            Framework.nativeSetBackgroundTiles(
                enabledSwitch.isChecked,
                currentUrl,
                cacheSizeSlider.value.toInt(),
                opacitySlider.value.toInt(),
            )
        }
        super.onPause()
    }

    // Grey out the URL / cache-size / opacity fields when the layer is disabled; their values are kept.
    private fun updateFieldsEnabled(enabled: Boolean) {
        urlField.isEnabled = enabled
        cacheSizeSlider.isEnabled = enabled
        opacitySlider.isEnabled = enabled
    }

    private fun refreshValidation() {
        urlInputLayout.error = if (isConfigValid) null else getString(R.string.pref_bg_tiles_url_error)
    }
}

private fun Slider.bindValueLabel(label: TextView) {
    addOnChangeListener { _, value, _ ->
        label.text = "${value.toInt()}"
    }
}
