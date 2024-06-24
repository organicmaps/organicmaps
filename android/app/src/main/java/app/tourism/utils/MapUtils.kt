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