package app.tourism.ui.utils

import android.app.AlertDialog
import android.content.Context
import android.content.Context.LOCATION_SERVICE
import android.content.Intent
import android.location.LocationManager
import android.provider.Settings
import androidx.core.content.ContextCompat.startActivity
import app.organicmaps.R

fun enableLocation(context: Context, onSuccess: () -> Unit) {
    val locationManager = context.getSystemService(LOCATION_SERVICE) as LocationManager
    if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
        showEnableLocationDialog(context)
    } else {
        onSuccess()
    }
}

private fun showEnableLocationDialog(context: Context) {
    AlertDialog.Builder(context)
        .setTitle(context.getString(R.string.enable_location))
        .setMessage(context.getString(R.string.enable_location_longer))
        .setPositiveButton(context.getString(R.string.ok)) { _, _ ->
            // Open location settings
            val intent = Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)
            startActivity(context, intent, null)
        }
        .setNegativeButton(context.getString(R.string.cancel), null)
        .create()
        .show()
}
