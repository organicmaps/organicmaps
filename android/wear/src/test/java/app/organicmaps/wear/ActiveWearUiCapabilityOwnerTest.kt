package app.organicmaps.wear

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class ActiveWearUiCapabilityOwnerTest {
    private data class Change(val active: Boolean, val complete: (Boolean) -> Unit)
    private val pending = mutableListOf<Change>()
    private val owner = ActiveWearUiCapabilityOwner { active, done -> pending += Change(active, done) }

    @Test
    fun lateOldActivityAddCannotWithdrawNewActivity() {
        val first = Any()
        val second = Any()
        owner.activate(first)
        owner.deactivate(first)
        owner.activate(second)
        assertEquals(true, pending.single().active)
        pending.removeAt(0).complete(true)
        assertTrue(pending.isEmpty())
        owner.deactivate(second)
        assertEquals(false, pending.single().active)
    }

    @Test
    fun lateRemovalReconcilesNewActivity() {
        val first = Any()
        val second = Any()
        owner.activate(first)
        pending.removeAt(0).complete(true)
        owner.deactivate(first)
        owner.activate(second)
        pending.removeAt(0).complete(true)
        assertEquals(true, pending.single().active)
        pending.removeAt(0).complete(true)
        assertTrue(pending.isEmpty())
    }

    @Test
    fun failedTaskCanRetryWithoutLosingConsumer() {
        owner.activate(this)
        pending.removeAt(0).complete(false)
        assertTrue(pending.isEmpty())
        owner.retry()
        assertEquals(true, pending.single().active)
        pending.removeAt(0).complete(true)
        assertTrue(pending.isEmpty())
    }
}
