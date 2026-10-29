package app.organicmaps.wear

import android.content.Context
import android.net.Uri
import android.os.Looper
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.tasks.TaskCompletionSource
import com.google.android.gms.tasks.Tasks
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.CapabilityInfo
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataItem
import com.google.android.gms.wearable.DataItemBuffer
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.MessageClient
import com.google.android.gms.wearable.MessageEvent
import com.google.android.gms.wearable.Node
import com.google.android.gms.wearable.PutDataMapRequest
import com.google.android.gms.wearable.Wearable
import java.util.concurrent.TimeUnit
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.MockedStatic
import org.mockito.Mockito.any
import org.mockito.Mockito.eq
import org.mockito.Mockito.mock
import org.mockito.Mockito.mockStatic
import org.mockito.Mockito.never
import org.mockito.Mockito.verify
import org.mockito.Mockito.`when`
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config
import org.robolectric.annotation.LooperMode

@RunWith(RobolectricTestRunner::class)
@LooperMode(LooperMode.Mode.PAUSED)
@Config(sdk = [35])
class MainActivityRobolectricTest {
    private lateinit var dataClient: DataClient
    private lateinit var messageClient: MessageClient
    private lateinit var capabilityClient: CapabilityClient
    private lateinit var mockedWearable: MockedStatic<Wearable>
    private lateinit var reachablePhone: CapabilityInfo
    private var phoneOnline = false
    private val mainLooper get() = shadowOf(Looper.getMainLooper())

    @Before
    fun setUp() {
        ActiveWearUiCapabilityRegistry.resetForTests()
        dataClient = mock(DataClient::class.java)
        messageClient = mock(MessageClient::class.java)
        capabilityClient = mock(CapabilityClient::class.java)
        stubClientTasks()
        mockedWearable = mockStatic(Wearable::class.java)
        mockedWearable.`when`<DataClient> { Wearable.getDataClient(any(Context::class.java)) }.thenReturn(dataClient)
        mockedWearable.`when`<MessageClient> {
            Wearable.getMessageClient(any(Context::class.java))
        }.thenReturn(messageClient)
        mockedWearable.`when`<CapabilityClient> { Wearable.getCapabilityClient(any(Context::class.java)) }
            .thenReturn(capabilityClient)
    }

    @After
    fun tearDown() {
        mockedWearable.close()
        ActiveWearUiCapabilityRegistry.resetForTests()
    }

    @Test
    fun phoneCapabilityReconnectReadsCachedModeAndAllowsLiveDetails() {
        val controller = Robolectric.buildActivity(MainActivity::class.java).setup()
        val activity = controller.get()
        mainLooper.idle()
        verify(dataClient, never()).getDataItems(any(Uri::class.java), eq(DataClient.FILTER_LITERAL))

        // No DataItem event or second onResume: the phone capability alone changes.
        phoneOnline = true
        activity.onCapabilityChanged(reachablePhone)
        mainLooper.idle()
        verify(dataClient).getDataItems(any(Uri::class.java), eq(DataClient.FILTER_LITERAL))
        deliverDetails(activity, "Rue Lamartine")
        assertTrue(containsText(activity.window.decorView, "Rue Lamartine"))
        controller.pause().stop().destroy()
    }

    @Test
    fun laterMessageExtendsExpiryAndPauseClearsViews() {
        phoneOnline = true
        val controller = Robolectric.buildActivity(MainActivity::class.java).setup()
        val activity = controller.get()
        mainLooper.idle()

        deliverDetails(activity, "First street")
        assertTrue(containsText(activity.window.decorView, "First street"))
        mainLooper.idleFor(3, TimeUnit.SECONDS)
        deliverDetails(activity, "Second street")
        mainLooper.idleFor(3, TimeUnit.SECONDS)
        assertTrue(containsText(activity.window.decorView, "Second street"))
        mainLooper.idleFor(2, TimeUnit.SECONDS)
        assertFalse(containsText(activity.window.decorView, "Second street"))

        deliverDetails(activity, "Before pause")
        controller.pause()
        assertFalse(containsText(activity.window.decorView, "Before pause"))
        controller.stop().destroy()
    }

    @Test
    fun oldActivityAddCompletionCannotRemoveNewActivityCapability() {
        val pendingAdd = TaskCompletionSource<Void>()
        `when`(capabilityClient.addLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI))
            .thenReturn(pendingAdd.task)

        val oldActivity = Robolectric.buildActivity(MainActivity::class.java).setup()
        mainLooper.idle()
        oldActivity.pause().stop().destroy()
        val newActivity = Robolectric.buildActivity(MainActivity::class.java).setup()
        mainLooper.idle()
        pendingAdd.setResult(null)
        mainLooper.idle()

        verify(capabilityClient, never()).removeLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI)
        newActivity.pause()
        mainLooper.idle()
        verify(capabilityClient).removeLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI)
        newActivity.stop().destroy()
    }

    private fun stubClientTasks() {
        val successful = TaskCompletionSource<Void>().apply { setResult(null) }.task
        `when`(messageClient.addListener(any(MessageClient.OnMessageReceivedListener::class.java)))
            .thenReturn(successful)
        `when`(messageClient.removeListener(any(MessageClient.OnMessageReceivedListener::class.java)))
            .thenReturn(Tasks.forResult(true))
        `when`(
            dataClient.addListener(
                any(DataClient.OnDataChangedListener::class.java),
                any(Uri::class.java),
                eq(DataClient.FILTER_LITERAL),
            ),
        ).thenReturn(successful)
        `when`(dataClient.removeListener(any(DataClient.OnDataChangedListener::class.java)))
            .thenReturn(Tasks.forResult(true))
        `when`(
            capabilityClient.addListener(
                any(CapabilityClient.OnCapabilityChangedListener::class.java),
                eq(WearNavigationData.CAPABILITY_PHONE_APP),
            ),
        ).thenReturn(successful)
        `when`(
            capabilityClient.removeListener(
                any(CapabilityClient.OnCapabilityChangedListener::class.java),
                eq(WearNavigationData.CAPABILITY_PHONE_APP),
            ),
        ).thenReturn(Tasks.forResult(true))
        `when`(capabilityClient.addLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI))
            .thenReturn(successful)
        `when`(capabilityClient.removeLocalCapability(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI))
            .thenReturn(successful)

        val phone = mock(Node::class.java)
        `when`(phone.id).thenReturn("phone-id")
        `when`(phone.isNearby).thenReturn(true)
        reachablePhone = mock(CapabilityInfo::class.java)
        `when`(reachablePhone.name).thenReturn(WearNavigationData.CAPABILITY_PHONE_APP)
        `when`(reachablePhone.nodes).thenReturn(setOf(phone))
        val unreachable = mock(CapabilityInfo::class.java)
        `when`(unreachable.nodes).thenReturn(emptySet())
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_PHONE_APP,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenAnswer {
            Tasks.forResult(if (phoneOnline) reachablePhone else unreachable)
        }
        `when`(dataClient.getDataItems(any(Uri::class.java), eq(DataClient.FILTER_LITERAL))).thenAnswer {
            Tasks.forResult(cachedNavigationMode())
        }
    }

    private fun cachedNavigationMode(): DataItemBuffer {
        val request = PutDataMapRequest.create(WearNavigationData.PATH_NAVIGATION_STATE)
        WearNavigationDataMapCodec.encode(request.dataMap, WearNavigationMode.NAVIGATION)
        val dataItem = mock(DataItem::class.java)
        `when`(dataItem.uri).thenReturn(request.asPutDataRequest().uri)
        `when`(dataItem.data).thenReturn(request.asPutDataRequest().data)
        val buffer = mock(DataItemBuffer::class.java)
        `when`(buffer.count).thenReturn(1)
        `when`(buffer[0]).thenReturn(dataItem)
        return buffer
    }

    private fun deliverDetails(activity: MainActivity, street: String) {
        val data = DataMap()
        WearNavigationDetailsDataMapCodec.encode(data, WearNavigationDetails(nextStreet = street))
        val event = mock(MessageEvent::class.java)
        `when`(event.path).thenReturn(WearNavigationData.PATH_NAVIGATION_DETAILS)
        `when`(event.data).thenReturn(data.toByteArray())
        activity.onMessageReceived(event)
    }

    private fun containsText(view: View, text: String): Boolean {
        if (view is TextView && view.text.toString() == text) return true
        if (view is ViewGroup) {
            return (0 until view.childCount).any { containsText(view.getChildAt(it), text) }
        }
        return false
    }
}
