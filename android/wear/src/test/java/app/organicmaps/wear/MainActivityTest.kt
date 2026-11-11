package app.organicmaps.wear

import android.widget.TextView
import app.organicmaps.wear.protocol.WearNavigationState
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import com.google.android.gms.tasks.OnFailureListener
import com.google.android.gms.tasks.OnSuccessListener
import com.google.android.gms.tasks.Task
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.CapabilityInfo
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataItem
import com.google.android.gms.wearable.DataItemBuffer
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.Node
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.mockito.ArgumentMatchers.any
import org.mockito.ArgumentMatchers.anyInt
import org.mockito.ArgumentMatchers.anyString
import org.mockito.Mockito.mock
import org.mockito.Mockito.verify
import org.mockito.Mockito.`when`
import java.io.IOException

class MainActivityTest {
    private lateinit var activity: MainActivity
    private lateinit var subtitle: TextView
    private lateinit var dataClient: DataClient
    private lateinit var capabilityClient: CapabilityClient

    private var scheduledDelay: Long? = null
    private var scheduledRunnable: Runnable? = null
    private var scheduleCount = 0
    private var cancelCount = 0

    @Before
    fun setUp() {
        activity = MainActivity()
        subtitle = mock(TextView::class.java)
        dataClient = mock(DataClient::class.java)
        capabilityClient = mock(CapabilityClient::class.java)

        activity.subtitle = subtitle
        activity.dataClient = dataClient
        activity.capabilityClient = capabilityClient

        activity.retryScheduler = { delay, runnable ->
            scheduledDelay = delay
            scheduledRunnable = runnable
            scheduleCount++
        }
        activity.retryCanceller = { runnable ->
            if (scheduledRunnable == runnable) {
                scheduledRunnable = null
            }
            cancelCount++
        }

        // By default, addListener succeeds so onResume proceeds to refreshNavigationState
        `when`(dataClient.addListener(any(), any(), anyInt()))
            .thenReturn(immediateSuccessTask(null))
        `when`(dataClient.removeListener(any()))
            .thenReturn(immediateSuccessTask(true))
    }

    @Test
    fun capabilityQueryFailure_schedulesRetry_andRecoversOnSuccessfulRead() {
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateFailureTask(IOException("Transient capability failure")))

        activity.onResume()

        verify(subtitle).setText(R.string.wear_no_navigation_message)
        assertTrue("Retry must be pending after capability read failure", activity.isRetryPending())
        assertNotNull("Retry action must be scheduled", scheduledRunnable)
        assertEquals(MainActivity.RETRY_DELAY_MS, scheduledDelay)

        // Companion phone is reachable and has active navigation
        val companion = mockNode("companion-phone-1", true)
        val capabilityInfo = mockCapability(setOf(companion))
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateSuccessTask(capabilityInfo))

        val dataItemBuffer = mock(DataItemBuffer::class.java)
        `when`(dataItemBuffer.count).thenReturn(1)
        val dataItem = mockNavigationDataItem()
        `when`(dataItemBuffer[0]).thenReturn(dataItem)
        `when`(dataClient.getDataItems(any(), anyInt()))
            .thenReturn(immediateSuccessTask(dataItemBuffer))

        // Execute scheduled retry
        val pendingAction = scheduledRunnable!!
        pendingAction.run()

        assertFalse("Retry must not remain pending after recovery", activity.isRetryPending())
        verify(subtitle).setText(R.string.wear_navigation_active_message)
        verify(dataItemBuffer).release()
    }

    @Test
    fun dataItemQueryFailure_schedulesRetry_andRecoversOnSuccessfulRead() {
        val companion = mockNode("companion-phone-1", true)
        val capabilityInfo = mockCapability(setOf(companion))
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateSuccessTask(capabilityInfo))
        `when`(dataClient.getDataItems(any(), anyInt()))
            .thenReturn(immediateFailureTask(IOException("Transient DataItem failure")))

        activity.onResume()

        verify(subtitle).setText(R.string.wear_no_navigation_message)
        assertTrue("Retry must be pending after DataItem read failure", activity.isRetryPending())
        assertNotNull("Retry action must be scheduled", scheduledRunnable)
        assertEquals(MainActivity.RETRY_DELAY_MS, scheduledDelay)

        // Restore successful DataItem query
        val dataItemBuffer = mock(DataItemBuffer::class.java)
        `when`(dataItemBuffer.count).thenReturn(1)
        val dataItem = mockNavigationDataItem()
        `when`(dataItemBuffer[0]).thenReturn(dataItem)
        `when`(dataClient.getDataItems(any(), anyInt()))
            .thenReturn(immediateSuccessTask(dataItemBuffer))

        // Execute scheduled retry
        val pendingAction = scheduledRunnable!!
        pendingAction.run()

        assertFalse("Retry must not remain pending after recovery", activity.isRetryPending())
        verify(subtitle).setText(R.string.wear_navigation_active_message)
        verify(dataItemBuffer).release()
    }

    @Test
    fun retriesAreCoalesced_andCancelledOnPause() {
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateFailureTask(IOException("Transient capability failure")))

        activity.onResume()

        assertTrue(activity.isRetryPending())
        assertEquals(1, scheduleCount)
        val firstRunnable = scheduledRunnable
        assertNotNull(firstRunnable)

        // Another refresh attempt while retry is pending (e.g. data event) must not duplicate retry
        val dataEvents = mock(com.google.android.gms.wearable.DataEventBuffer::class.java)
        val changeEvent = mock(com.google.android.gms.wearable.DataEvent::class.java)
        `when`(changeEvent.type).thenReturn(com.google.android.gms.wearable.DataEvent.TYPE_CHANGED)
        `when`(dataEvents.iterator()).thenReturn(mutableListOf(changeEvent).iterator())

        activity.onDataChanged(dataEvents)

        // After the new refresh failed again, exactly one retry should be scheduled (coalesced)
        assertTrue(activity.isRetryPending())
        assertEquals("Subsequent failure must schedule a retry without piling up", 2, scheduleCount)
        assertEquals("Prior retry was cancelled when new refresh began", 1, cancelCount)

        // Pausing must cancel the pending retry
        activity.onPause()

        assertFalse("Retry must be cancelled on pause", activity.isRetryPending())
        assertNull("Pending runnable must be cleared on pause", scheduledRunnable)
        assertEquals(2, cancelCount)

        // Executing the cancelled runnable after pause must not refresh or reschedule
        firstRunnable?.run()
        assertFalse(activity.isRetryPending())
    }

    @Test
    fun staleFailureCompletion_doesNotScheduleRetry() {
        var failureCallback: OnFailureListener? = null
        val deferredTask = mock(Task::class.java) as Task<CapabilityInfo>
        `when`(deferredTask.addOnSuccessListener(any())).thenReturn(deferredTask)
        `when`(deferredTask.addOnFailureListener(any())).thenAnswer { invocation ->
            failureCallback = invocation.getArgument(0)
            deferredTask
        }
        `when`(capabilityClient.getCapability(anyString(), anyInt())).thenReturn(deferredTask)

        activity.onResume()

        // Before task completes, activity is paused (stale generation)
        activity.onPause()

        // Task now fails asynchronously after pause
        failureCallback?.onFailure(IOException("Late failure"))

        assertFalse("Stale task failure must not schedule retry", activity.isRetryPending())
        assertNull(scheduledRunnable)
    }

    @Test
    fun noCompanion_rendersNormal_andDoesNotScheduleRetry() {
        val emptyCapability = mockCapability(emptySet())
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateSuccessTask(emptyCapability))

        activity.onResume()

        verify(subtitle).setText(R.string.wear_no_navigation_message)
        assertFalse("No companion is a valid normal state, not a failure to retry", activity.isRetryPending())
        assertNull(scheduledRunnable)
    }

    @Test
    fun successfulRead_rendersNavigationActiveMessage() {
        val companion = mockNode("companion-phone-1", true)
        val capabilityInfo = mockCapability(setOf(companion))
        `when`(capabilityClient.getCapability(anyString(), anyInt()))
            .thenReturn(immediateSuccessTask(capabilityInfo))

        val dataItemBuffer = mock(DataItemBuffer::class.java)
        `when`(dataItemBuffer.count).thenReturn(1)
        val dataItem = mockNavigationDataItem()
        `when`(dataItemBuffer[0]).thenReturn(dataItem)
        `when`(dataClient.getDataItems(any(), anyInt()))
            .thenReturn(immediateSuccessTask(dataItemBuffer))

        activity.onResume()

        verify(subtitle).setText(R.string.wear_navigation_active_message)
        assertFalse(activity.isRetryPending())
        verify(dataItemBuffer).release()
    }

    private fun mockNode(id: String, nearby: Boolean): Node {
        val node = mock(Node::class.java)
        `when`(node.id).thenReturn(id)
        `when`(node.isNearby).thenReturn(nearby)
        return node
    }

    private fun mockCapability(nodes: Set<Node>): CapabilityInfo {
        val cap = mock(CapabilityInfo::class.java)
        `when`(cap.nodes).thenReturn(nodes)
        return cap
    }

    private fun mockNavigationDataItem(): DataItem {
        val dataItem = mock(DataItem::class.java)
        val dataMap = DataMap()
        WearNavigationDataMapCodec.encode(dataMap, WearNavigationState.navigation())
        `when`(dataItem.data).thenReturn(dataMap.toByteArray())
        return dataItem
    }

    private fun <T> immediateSuccessTask(result: T): Task<T> {
        val task = mock(Task::class.java) as Task<T>
        `when`(task.addOnSuccessListener(any())).thenAnswer { invocation ->
            val listener = invocation.getArgument<OnSuccessListener<T>>(0)
            listener.onSuccess(result)
            task
        }
        `when`(task.addOnFailureListener(any())).thenReturn(task)
        return task
    }

    private fun <T> immediateFailureTask(exception: Exception): Task<T> {
        val task = mock(Task::class.java) as Task<T>
        `when`(task.addOnSuccessListener(any())).thenReturn(task)
        `when`(task.addOnFailureListener(any())).thenAnswer { invocation ->
            val listener = invocation.getArgument<OnFailureListener>(0)
            listener.onFailure(exception)
            task
        }
        return task
    }
}
