package app.organicmaps.wear

import com.google.android.gms.wearable.DataEvent
import com.google.android.gms.wearable.DataEventBuffer
import com.google.android.gms.wearable.DataMapItem
import com.google.android.gms.wearable.WearableListenerService

class WearDataListenerService : WearableListenerService() {

    override fun onDataChanged(dataEvents: DataEventBuffer) {
        for (event in dataEvents) {
            if (event.type == DataEvent.TYPE_CHANGED && event.dataItem.uri.path == "/navigation/status") {
                val dataMap = DataMapItem.fromDataItem(event.dataItem).dataMap
                
                val newState = NavigationState(
                    distToTurn = dataMap.getString("distToTurn") ?: "",
                    nextStreet = dataMap.getString("nextStreet") ?: "",
                    carDirection = dataMap.getInt("carDirection"),
                    exitNum = dataMap.getInt("exitNum"),
                    isActive = dataMap.getBoolean("active", true)
                )
                NavigationStateHolder.update(newState)
            }
        }
    }
}
