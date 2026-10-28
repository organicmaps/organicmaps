package app.organicmaps.sdk.wear.gms

import android.os.Looper
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.gms.WearNavigationDetailsDataMapCodec
import com.google.android.gms.common.api.ApiException
import com.google.android.gms.common.api.CommonStatusCodes
import com.google.android.gms.common.api.Status
import com.google.android.gms.tasks.TaskCompletionSource
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.CapabilityInfo
import com.google.android.gms.wearable.DataClient
import com.google.android.gms.wearable.DataItem
import com.google.android.gms.wearable.DataMap
import com.google.android.gms.wearable.MessageClient
import com.google.android.gms.wearable.Node
import com.google.android.gms.wearable.PutDataRequest
import java.util.concurrent.TimeUnit
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentCaptor
import org.mockito.Mockito.any
import org.mockito.Mockito.eq
import org.mockito.Mockito.mock
import org.mockito.Mockito.times
import org.mockito.Mockito.verify
import org.mockito.Mockito.`when`
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config
import org.robolectric.annotation.LooperMode
import org.robolectric.shadows.ShadowPausedLooper

@RunWith(RobolectricTestRunner::class)
@LooperMode(LooperMode.Mode.PAUSED)
@Config(sdk = [35], manifest = Config.NONE)
class GmsWearNavigationPublisherRobolectricTest {
    private lateinit var dataClient: DataClient
    private lateinit var messageClient: MessageClient
    private lateinit var capabilityClient: CapabilityClient
    private lateinit var listenerRegistration: TaskCompletionSource<Void>
    private val mainLooper get() = shadowOf(Looper.getMainLooper())

    @Before
    fun setUp() {
        dataClient = mock(DataClient::class.java)
        messageClient = mock(MessageClient::class.java)
        capabilityClient = mock(CapabilityClient::class.java)

        listenerRegistration = TaskCompletionSource()
        `when`(
            capabilityClient.addListener(
                any(CapabilityClient.OnCapabilityChangedListener::class.java),
                eq(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI),
            ),
        ).thenReturn(listenerRegistration.task)

        val writtenItem = TaskCompletionSource<DataItem>().apply {
            setResult(mock(DataItem::class.java))
        }.task
        `when`(dataClient.putDataItem(any(PutDataRequest::class.java)))
            .thenReturn(writtenItem)

        val sent = TaskCompletionSource<Int>().apply { setResult(1) }.task
        `when`(
            messageClient.sendMessage(
                any(String::class.java),
                eq(WearNavigationData.PATH_NAVIGATION_DETAILS),
                any(ByteArray::class.java),
            ),
        ).thenReturn(sent)
    }

    @Test
    fun apiNotConnectedDiscoveryDoesNotRetryUntilNextNavigationStart() {
        val first = TaskCompletionSource<CapabilityInfo>()
        val second = TaskCompletionSource<CapabilityInfo>()

        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(first.task, second.task)

        val publisher = startPublisher()
        verifyLookupCount(1)

        first.setException(ApiException(Status(CommonStatusCodes.API_NOT_CONNECTED)))
        mainLooper.idle()
        mainLooper.idleFor(4L, TimeUnit.SECONDS)

        // The unavailable Wear API must not trigger periodic discovery.
        verifyLookupCount(1)

        onMainThread {
            publisher.publish(WearNavigationMode.NORMAL)
            publisher.publish(WearNavigationMode.NAVIGATION)
        }

        // A new route may attempt discovery again.
        verifyLookupCount(2)
    }

    @Test
    fun apiNotConnectedListenerRegistrationDoesNotRetryUntilNextNavigationStart() {
        val nextRegistration = TaskCompletionSource<Void>()

        `when`(
            capabilityClient.addListener(
                any(CapabilityClient.OnCapabilityChangedListener::class.java),
                eq(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI),
            ),
        ).thenReturn(listenerRegistration.task, nextRegistration.task)

        val empty = mock(CapabilityInfo::class.java)
        `when`(empty.nodes).thenReturn(emptySet())

        val lookup = TaskCompletionSource<CapabilityInfo>().apply {
            setResult(empty)
        }.task

        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(lookup)

        val publisher = createPublisher()
        onMainThread { publisher.publish(WearNavigationMode.NAVIGATION) }
        mainLooper.idle()
        verifyListenerRegistrationCount(1)

        listenerRegistration.setException(
            ApiException(Status(CommonStatusCodes.API_NOT_CONNECTED)),
        )
        mainLooper.idle()
        mainLooper.idleFor(4L, TimeUnit.SECONDS)

        // No repeating registration attempts for a permanently unavailable API.
        verifyListenerRegistrationCount(1)

        onMainThread {
            publisher.publish(WearNavigationMode.NORMAL)
            publisher.publish(WearNavigationMode.NAVIGATION)
        }

        // A later route can try registration again.
        verifyListenerRegistrationCount(2)
    }

    @Test
    fun failedListenerRegistrationRetriesAndRefreshesEmptyMembership() {
        val secondRegistration = TaskCompletionSource<Void>()
        `when`(
            capabilityClient.addListener(
                any(CapabilityClient.OnCapabilityChangedListener::class.java),
                eq(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI),
            ),
        ).thenReturn(listenerRegistration.task, secondRegistration.task)

        val empty = mock(CapabilityInfo::class.java)
        `when`(empty.nodes).thenReturn(emptySet())

        val watch = mock(Node::class.java)
        `when`(watch.id).thenReturn("watch-1")
        val reachable = mock(CapabilityInfo::class.java)
        `when`(reachable.nodes).thenReturn(setOf(watch))

        val first = TaskCompletionSource<CapabilityInfo>().apply { setResult(empty) }.task
        val refreshed = TaskCompletionSource<CapabilityInfo>().apply { setResult(reachable) }.task
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(first, refreshed)

        val publisher = createPublisher()
        onMainThread {
            publisher.publish(WearNavigationMode.NAVIGATION)
            publisher.publish(WearNavigationDetails(nextStreet = "Recovered route"))
        }
        mainLooper.idle()
        verifyLookupCount(1)

        listenerRegistration.setException(IllegalStateException("Listener registration failed"))
        mainLooper.idle()
        mainLooper.idleFor(1_999L, TimeUnit.MILLISECONDS)
        verifyListenerRegistrationCount(1)

        mainLooper.idleFor(1L, TimeUnit.MILLISECONDS)
        verifyListenerRegistrationCount(2)
        verifyLookupCount(1)

        secondRegistration.setResult(null)
        mainLooper.idle()
        verifyLookupCount(2)

        val packet = ArgumentCaptor.forClass(ByteArray::class.java)
        verify(messageClient).sendMessage(
            eq("watch-1"),
            eq(WearNavigationData.PATH_NAVIGATION_DETAILS),
            packet.capture(),
        )
        val delivered = WearNavigationDetailsDataMapCodec.decode(DataMap.fromByteArray(packet.value))
        assertEquals("Recovered route", delivered?.nextStreet)
    }

    @Test
    fun navigationStopCancelsPendingListenerRegistrationRetry() {
        val empty = mock(CapabilityInfo::class.java)
        `when`(empty.nodes).thenReturn(emptySet())
        val lookup = TaskCompletionSource<CapabilityInfo>().apply { setResult(empty) }.task
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(lookup)

        val publisher = createPublisher()
        onMainThread { publisher.publish(WearNavigationMode.NAVIGATION) }
        mainLooper.idle()

        listenerRegistration.setException(IllegalStateException("Listener registration failed"))
        mainLooper.idle()
        onMainThread { publisher.publish(WearNavigationMode.NORMAL) }

        mainLooper.idleFor(2L, TimeUnit.SECONDS)
        verifyListenerRegistrationCount(1)
    }

    @Test
    fun failedInitialLookupRetriesAfterTwoSecondsAndDeliversTheLatestDetails() {
        val first = TaskCompletionSource<CapabilityInfo>()
        val retry = TaskCompletionSource<CapabilityInfo>()
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(first.task, retry.task)

        val publisher = startPublisher()
        onMainThread { publisher.publish(WearNavigationDetails(nextStreet = "Before retry")) }
        verifyLookupCount(1)

        first.setException(IllegalStateException("Transient Data Layer lookup failure"))
        mainLooper.idle()
        mainLooper.idleFor(1_999L, TimeUnit.MILLISECONDS)
        verifyLookupCount(1)

        // The current location can change while capability discovery is unavailable.
        onMainThread { publisher.publish(WearNavigationDetails(nextStreet = "After retry")) }
        mainLooper.idleFor(1L, TimeUnit.MILLISECONDS)
        verifyLookupCount(2)

        val watch = mock(Node::class.java)
        `when`(watch.id).thenReturn("watch-1")
        val reachable = mock(CapabilityInfo::class.java)
        `when`(reachable.nodes).thenReturn(setOf(watch))
        retry.setResult(reachable)
        mainLooper.idle()

        val packet = ArgumentCaptor.forClass(ByteArray::class.java)
        verify(messageClient).sendMessage(
            eq("watch-1"),
            eq(WearNavigationData.PATH_NAVIGATION_DETAILS),
            packet.capture(),
        )
        val delivered = WearNavigationDetailsDataMapCodec.decode(DataMap.fromByteArray(packet.value))
        assertEquals("After retry", delivered?.nextStreet)
    }

    @Test
    fun successfulEmptyDiscoveryIsCachedInsteadOfRetriedOnGpsUpdates() {
        val emptyCapability = mock(CapabilityInfo::class.java)
        `when`(emptyCapability.nodes).thenReturn(emptySet())
        val successful = TaskCompletionSource<CapabilityInfo>().apply { setResult(emptyCapability) }.task
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(successful)

        val publisher = startPublisher()
        repeat(10) { i ->
            onMainThread { publisher.publish(WearNavigationDetails(nextStreet = "Fix $i")) }
            mainLooper.idleFor(200L, TimeUnit.MILLISECONDS)
        }
        mainLooper.idleFor(2L, TimeUnit.SECONDS)
        verifyLookupCount(1)
    }

    @Test
    fun navigationStopCancelsPendingDiscoveryRetry() {
        val first = TaskCompletionSource<CapabilityInfo>()
        `when`(
            capabilityClient.getCapability(
                WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
                CapabilityClient.FILTER_REACHABLE,
            ),
        ).thenReturn(first.task)

        val publisher = startPublisher()
        first.setException(IllegalStateException("Transient failure"))
        mainLooper.idle()
        onMainThread { publisher.publish(WearNavigationMode.NORMAL) }
        mainLooper.idleFor(2L, TimeUnit.SECONDS)
        verifyLookupCount(1)
    }

    private fun createPublisher(): GmsWearNavigationPublisher {
        lateinit var publisher: GmsWearNavigationPublisher
        onMainThread {
            publisher =
                GmsWearNavigationPublisher(
                    context = RuntimeEnvironment.getApplication(),
                    dataClient = dataClient,
                    messageClient = messageClient,
                    capabilityClient = capabilityClient,
                )
        }
        return publisher
    }

    private fun verifyListenerRegistrationCount(expected: Int) {
        verify(capabilityClient, times(expected)).addListener(
            any(CapabilityClient.OnCapabilityChangedListener::class.java),
            eq(WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI),
        )
    }

    private fun startPublisher(): GmsWearNavigationPublisher {
        lateinit var publisher: GmsWearNavigationPublisher
        onMainThread {
            publisher =
                GmsWearNavigationPublisher(
                    context = RuntimeEnvironment.getApplication(),
                    dataClient = dataClient,
                    messageClient = messageClient,
                    capabilityClient = capabilityClient,
                )
        }
        // Complete listener registration before the route starts. Otherwise the registration
        // callback and the initial lookup can legitimately race and issue two lookups.
        listenerRegistration.setResult(null)
        mainLooper.idle()
        onMainThread { publisher.publish(WearNavigationMode.NAVIGATION) }
        mainLooper.idle()
        return publisher
    }

    private fun verifyLookupCount(expected: Int) {
        verify(capabilityClient, times(expected)).getCapability(
            WearNavigationData.CAPABILITY_ACTIVE_WATCH_UI,
            CapabilityClient.FILTER_REACHABLE,
        )
    }

    private fun onMainThread(action: () -> Unit) {
        (mainLooper as ShadowPausedLooper).postSync(Runnable { action() })
    }
}
