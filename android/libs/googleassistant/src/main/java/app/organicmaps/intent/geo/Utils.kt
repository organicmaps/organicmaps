package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import app.organicmaps.sdk.api.ApiController

private val invalidCoordinatesTemplate = doubleArrayOf(Double.NaN, Double.NaN)

internal fun getCoordinates(intent: Intent): DoubleArray {
    val data = intent.data ?: return invalidCoordinates()
    val latLon = ApiController.nativeParseLatLon(data.toString())
    if (latLon == null || latLon.size != 2) return invalidCoordinates()

    return latLon
}

internal fun getQueryParameters(intent: Intent): Map<String, String> {
    val data = intent.data ?: return emptyMap()
    val schemeSpecificPart = data.encodedSchemeSpecificPart ?: return emptyMap()
    val querySeparatorIndex = schemeSpecificPart.indexOf('?')
    if (querySeparatorIndex < 0 || querySeparatorIndex + 1 >= schemeSpecificPart.length) {
        return emptyMap()
    }

    val query = schemeSpecificPart.substring(querySeparatorIndex + 1)
    val parameters = mutableMapOf<String, String>()
    for (pair in query.split('&')) {
        val equalsIndex = pair.indexOf('=')
        if (equalsIndex <= 0) continue

        val key = Uri.decode(pair.substring(0, equalsIndex))
        val value = Uri.decode(pair.substring(equalsIndex + 1))
        parameters[key] = value
    }
    return parameters
}

private fun invalidCoordinates(): DoubleArray = invalidCoordinatesTemplate.copyOf()
