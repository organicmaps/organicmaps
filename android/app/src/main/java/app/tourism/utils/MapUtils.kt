package app.tourism.utils

import android.content.Context
import android.content.Intent
import androidx.core.content.ContextCompat
import app.organicmaps.MwmActivity
import app.tourism.data.dto.PlaceLocation

fun navigateToMap(context: Context, clearBackStack: Boolean = false) {
    val intent = Intent(context, MwmActivity::class.java)
    if (clearBackStack)
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK or Intent.FLAG_ACTIVITY_NEW_TASK)
    ContextCompat.startActivity(context, intent, null)
}

fun navigateToMapForRoute(context: Context, placeLocation: PlaceLocation) {
    val intent = Intent(context, MwmActivity::class.java)
    intent.putExtra("end_point", placeLocation)
    ContextCompat.startActivity(context, intent, null)
}

fun isInsideTajikistan(latitude: Double, longitude: Double): Boolean {
    val minLatitude = 36.4
    val maxLatitude = 41.3
    val minLongitude = 67.1
    val maxLongitude = 75.5

    if (latitude < minLatitude || latitude > maxLatitude) {
        return false
    }

    if (longitude < minLongitude || longitude > maxLongitude) {
        return false
    }

    return true
}
