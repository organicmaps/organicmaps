package app.organicmaps.wear

import com.google.android.gms.wearable.DataEvent
import com.google.android.gms.wearable.Node
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.mockito.Mockito.mock
import org.mockito.Mockito.`when`

class NavigationStateSourceTest {
    @Test
    fun emptyCollectionHasNoCompanion() {
        assertNull(NavigationStateSource.selectCompanion(emptyList()))
    }

    @Test
    fun nearbyNodeWins() {
        val remote = node("a", false)
        val nearby = node("z", true)

        assertEquals(nearby, NavigationStateSource.selectCompanion(listOf(remote, nearby)))
    }

    @Test
    fun nodeIdMakesSelectionDeterministic() {
        val first = node("a", true)
        val second = node("b", true)

        assertEquals(first, NavigationStateSource.selectCompanion(listOf(second, first)))
        assertEquals(first, NavigationStateSource.selectCompanion(listOf(first, second)))
    }

    @Test
    fun changedOrDeletedItemRequiresRefresh() {
        assertTrue(NavigationStateSource.requiresRefresh(listOf(event(DataEvent.TYPE_CHANGED))))
        assertTrue(NavigationStateSource.requiresRefresh(listOf(event(DataEvent.TYPE_DELETED))))
    }

    @Test
    fun emptyOrUnrelatedEventDoesNotRequireRefresh() {
        assertFalse(NavigationStateSource.requiresRefresh(emptyList()))
        assertFalse(NavigationStateSource.requiresRefresh(listOf(event(0))))
    }

    private fun node(id: String, nearby: Boolean): Node {
        val node = mock(Node::class.java)
        `when`(node.id).thenReturn(id)
        `when`(node.isNearby).thenReturn(nearby)
        return node
    }

    private fun event(type: Int): DataEvent {
        val event = mock(DataEvent::class.java)
        `when`(event.type).thenReturn(type)
        return event
    }
}
