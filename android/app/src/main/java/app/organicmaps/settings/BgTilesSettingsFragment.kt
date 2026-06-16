package app.organicmaps.settings

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.widget.SwitchCompat
import androidx.core.view.ViewCompat
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener
import com.google.android.material.slider.Slider
import com.google.android.material.textfield.TextInputEditText

// Settings sub-screen for the custom raster background tiles (XYZ {z}/{x}/{y} source).
// Values are persisted in the C++ core (not SharedPreferences); the layer is rendered when the
// "enabled" switch is on. Edits are applied together in onPause() when the user leaves the screen.
class BgTilesSettingsFragment : BaseMwmFragment() {

    private lateinit var enabledSwitch: SwitchCompat
    private lateinit var urlField: TextInputEditText
    private lateinit var cacheSizeSlider: Slider
    private lateinit var cacheSizeLabel: TextView
    private lateinit var opacitySlider: Slider
    private lateinit var opacityLabel: TextView

    private val currentUrl: String
        get() = urlField.text.toString().trim()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_bg_tiles_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(view, ScrollableContentInsetsListener(view))

        enabledSwitch = view.findViewById(R.id.bg_tiles_enabled)
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

        enabledSwitch.setOnCheckedChangeListener { _, isChecked -> updateFieldsEnabled(isChecked) }
        cacheSizeSlider.bindValueLabel(cacheSizeLabel)
        opacitySlider.bindValueLabel(opacityLabel)
    }

    override fun onPause() {
        Framework.nativeSetBackgroundTiles(
            enabledSwitch.isChecked,
            currentUrl,
            cacheSizeSlider.value.toInt(),
            opacitySlider.value.toInt(),
        )
        super.onPause()
    }

    // Grey out the URL / cache-size / opacity fields when the layer is disabled; their values are kept.
    private fun updateFieldsEnabled(enabled: Boolean) {
        urlField.isEnabled = enabled
        cacheSizeSlider.isEnabled = enabled
        opacitySlider.isEnabled = enabled
    }
}

private fun Slider.bindValueLabel(label: TextView) {
    addOnChangeListener { _, value, _ ->
        label.text = "${value.toInt()}"
    }
}
