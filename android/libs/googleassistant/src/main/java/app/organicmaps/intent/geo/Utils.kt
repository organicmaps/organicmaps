package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import app.organicmaps.sdk.api.ApiController
import app.organicmaps.sdk.util.Assert

internal fun getCoordinates(intent: Intent): DoubleArray {
    val data = intent.data ?: return invalidCoordinates()
    // Offline intents use the geo coordinate grammar.
    val url = if (data.scheme == "geo.offline") data.buildUpon().scheme("geo").build().toString() else data.toString()
    val latLon = ApiController.nativeParseLatLon(url) ?: return invalidCoordinates()
    Assert.debug(latLon.size == 2, "nativeParseLatLon must return two coordinates")

    return latLon
}

internal fun getQueryParameters(intent: Intent): Map<String, String> {
    val data = intent.data ?: return emptyMap()
    val query = data.encodedSchemeSpecificPart.substringAfter('?', "")
    if (query.isEmpty()) {
        return emptyMap()
    }
    val hierarchicalUri = Uri.Builder().encodedQuery(query).build()

    val parameters = mutableMapOf<String, String>()
    for (key in hierarchicalUri.queryParameterNames) {
        parameters[key] = hierarchicalUri.getQueryParameter(key) ?: continue
    }
    return parameters
}

private fun invalidCoordinates() = doubleArrayOf(Double.NaN, Double.NaN)
