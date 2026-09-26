package app.organicmaps.wear.protocol.gms

import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationMode
import com.google.android.gms.wearable.DataMap

/** Encodes and decodes the persistent Wear navigation mode using the DataMap format. */
object WearNavigationDataMapCodec {
    fun encode(dataMap: DataMap, mode: WearNavigationMode) {
        dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION)
        dataMap.putString(WearNavigationData.KEY_MODE, mode.name)
    }

    /**
     * Returns null when the peer protocol version differs or the payload is malformed, so an
     * incompatible peer degrades to no navigation state.
     */
    fun decode(dataMap: DataMap): WearNavigationMode? {
        if (dataMap.getInt(WearNavigationData.KEY_VERSION, -1) != WearNavigationData.VERSION) {
            return null
        }

        val modeName = dataMap.getString(WearNavigationData.KEY_MODE) ?: return null
        return WearNavigationMode.entries.find { it.name == modeName }
    }
}
