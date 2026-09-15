package app.organicmaps.intent.geo

import android.content.Intent
import androidx.core.net.toUri
import app.organicmaps.sdk.api.ApiController

private val invalidCoordinatesTemplate = doubleArrayOf(Double.NaN, Double.NaN)

internal fun getCoordinates(intent: Intent): DoubleArray {
    val data = intent.data ?: return invalidCoordinates()
    val latLon = ApiController.nativeParseLatLon(data.toString())
    if (latLon == null || latLon.size != 2) {
        return invalidCoordinates()
    }

    return latLon
}

internal fun getQueryParameters(intent: Intent): Map<String, String> {
    val data = intent.data ?: return emptyMap()
    val query = data.encodedQuery ?: return emptyMap()
    val hierarchicalUri = "https://${data.scheme}?$query".toUri()

    val parameters = mutableMapOf<String, String>()
    for (key in hierarchicalUri.queryParameterNames) {
        parameters[key] = hierarchicalUri.getQueryParameter(key) ?: continue
    }
    return parameters
}

private fun invalidCoordinates(): DoubleArray = invalidCoordinatesTemplate.copyOf()
