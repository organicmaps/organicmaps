package app.organicmaps.settings

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.view.ViewCompat
import androidx.core.widget.doAfterTextChanged
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout

// Settings sub-screen for the live traffic data source (TomTom vector flow tiles API key).
// The key is persisted in the C++ core (not SharedPreferences); an empty key falls back to
// the built-in traffic source. Edits are applied in onPause() when the user leaves the screen.
class TrafficSettingsFragment : BaseMwmFragment() {

    private lateinit var keyInputLayout: TextInputLayout
    private lateinit var keyField: TextInputEditText

    private val currentKey: String
        get() = keyField.text.toString().trim()

    private val isKeyValid: Boolean
        get() = currentKey.isEmpty() || Framework.nativeIsWellFormedTrafficApiKey(currentKey)

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_traffic_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(view, ScrollableContentInsetsListener(view))

        keyInputLayout = view.findViewById(R.id.traffic_api_key_input)
        keyField = view.findViewById(R.id.traffic_api_key)
        keyField.setText(Framework.nativeGetTrafficApiKey())
        keyField.doAfterTextChanged { refreshValidation() }

        refreshValidation()
    }

    override fun onPause() {
        if (isKeyValid) {
            Framework.nativeSetTrafficApiKey(currentKey)
        }
        super.onPause()
    }

    private fun refreshValidation() {
        keyInputLayout.error = if (isKeyValid) null else getString(R.string.pref_traffic_key_error)
    }
}
