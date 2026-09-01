package app.organicmaps.wear.protocol.gms

import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.WearNavigationState
import com.google.android.gms.wearable.DataMap

/** Encodes and decodes Wear navigation state using the Google Wear Data Layer DataMap format. */
object WearNavigationDataMapCodec {
    fun encode(dataMap: DataMap, state: WearNavigationState) {
        dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION)
        dataMap.putString(WearNavigationData.KEY_MODE, state.mode.name)
    }

    /**
     * Returns null when the peer protocol version differs or the payload is malformed, so an
     * incompatible peer degrades to no navigation state.
     */
    fun decode(dataMap: DataMap): WearNavigationState? {
        if (dataMap.getInt(WearNavigationData.KEY_VERSION, -1) != WearNavigationData.VERSION) {
            return null
        }

        val modeName = dataMap.getString(WearNavigationData.KEY_MODE) ?: return null

        val mode = WearNavigationMode.entries.find { it.name == modeName } ?: return null
        return WearNavigationState.of(mode)
    }
}
