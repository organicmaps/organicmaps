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
    val query = intent.data?.encodedSchemeSpecificPart?.substringAfter('?', "") ?: return emptyMap()
    val parameters = mutableMapOf<String, String>()
    // Split before decoding so escaped separators remain part of their key or value.
    for (pair in query.split('&')) {
        if (pair.isEmpty()) continue
        val key = Uri.decode(pair.substringBefore('=').replace('+', ' '))
        if (key !in parameters) {
            parameters[key] = Uri.decode(pair.substringAfter('=', "").replace('+', ' '))
        }
    }
    return parameters
}

private fun invalidCoordinates() = doubleArrayOf(Double.NaN, Double.NaN)
