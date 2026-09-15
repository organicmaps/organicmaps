package app.organicmaps.intent.geo

import app.organicmaps.sdk.api.ApiController
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

object TestApiControllerHelper {
    private val mLatLonByUrl = mutableMapOf<String, DoubleArray?>()

    fun stubLatLon(url: String, latLon: DoubleArray?) {
        mLatLonByUrl[url] = latLon
    }

    fun reset() {
        mLatLonByUrl.clear()
    }

    fun getLatLon(url: String): DoubleArray? = mLatLonByUrl[url]
}

@Implements(ApiController::class)
class ShadowApiController {
    companion object {
        @Implementation
        @JvmStatic
        fun nativeParseLatLon(url: String): DoubleArray? = TestApiControllerHelper.getLatLon(url)
    }
}
