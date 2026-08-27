package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import app.organicmaps.sdk.api.ApiController

internal fun getCoordinates(intent: Intent): DoubleArray {
    val data = intent.data ?: return invalidCoordinates()
    val latLon = ApiController.nativeParseLatLon(data.toString())
    if (latLon == null || latLon.size != 2) {
        return invalidCoordinates()
    }
    if (latLon[0] == 0.0 && latLon[1] == 0.0) {
        return invalidCoordinates()
    }

    return latLon
}

internal fun getQueryParameters(intent: Intent): Map<String, String> {
    val data = intent.data ?: return emptyMap()
    val query = data.encodedQuery ?: return emptyMap()
    val hierarchicalUri = Uri.Builder().encodedQuery(query).build()

    val parameters = mutableMapOf<String, String>()
    for (key in hierarchicalUri.queryParameterNames) {
        parameters[key] = hierarchicalUri.getQueryParameter(key) ?: continue
    }
    return parameters
}

private fun invalidCoordinates() = doubleArrayOf(Double.NaN, Double.NaN)
