package app.organicmaps.wear

import app.organicmaps.wear.protocol.WearNavigationData
import com.google.android.gms.wearable.DataEvent
import com.google.android.gms.wearable.Node

internal object NavigationStateSource {
    fun isPhoneCapability(name: String): Boolean = name == WearNavigationData.CAPABILITY_PHONE_APP

    fun selectCompanion(nodes: Collection<Node>): Node? =
        nodes.minWithOrNull(compareByDescending<Node> { it.isNearby }.thenBy { it.id })

    fun requiresRefresh(events: Iterable<DataEvent>): Boolean = events.any {
        it.type == DataEvent.TYPE_CHANGED || it.type == DataEvent.TYPE_DELETED
    }
}
