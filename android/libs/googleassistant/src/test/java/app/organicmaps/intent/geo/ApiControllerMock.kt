package app.organicmaps.intent.geo

import app.organicmaps.sdk.api.ApiController
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@Implements(ApiController::class)
@Suppress("UtilityClassWithPublicConstructor") // Robolectric shadows require an instantiable class, not an object.
class ApiControllerMock {
    companion object {
        private val mLatLonByUrl = mutableMapOf<String, DoubleArray?>()

        fun mockLatLon(url: String, latLon: DoubleArray?) {
            mLatLonByUrl[url] = latLon
        }

        fun reset() {
            mLatLonByUrl.clear()
        }

        @Implementation
        @JvmStatic
        fun nativeParseLatLon(url: String): DoubleArray? = mLatLonByUrl[url]
    }
}
