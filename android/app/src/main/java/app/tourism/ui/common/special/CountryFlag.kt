package app.tourism.ui.common.special

import android.view.LayoutInflater
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.viewinterop.AndroidView
import app.organicmaps.R
import com.hbb20.CountryCodePicker

@Composable
fun CountryFlag(modifier: Modifier = Modifier, countryCodeName: String) {
    AndroidView(
        modifier = Modifier.then(modifier),
        factory = { context ->
            val view = LayoutInflater.from(context)
                .inflate(R.layout.ccp_country_flag, null, false)
            val ccp = view.findViewById<CountryCodePicker>(R.id.ccp)
            ccp.setCountryForNameCode(countryCodeName)
            view
        }
    )
}