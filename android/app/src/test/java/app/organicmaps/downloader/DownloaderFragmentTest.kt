package app.organicmaps.downloader

import app.organicmaps.sdk.search.MapSearchListener
import org.junit.Test
import org.mockito.ArgumentMatchers.any
import org.mockito.Mockito.mock
import org.mockito.Mockito.never
import org.mockito.Mockito.verify

class DownloaderFragmentTest {
    @Test
    fun clearingQueryDropsPendingSearchResults() {
        val fragment = DownloaderFragment()
        val adapter = mock(DownloaderAdapter::class.java)
        setField(fragment, "mAdapter", adapter)
        setField(fragment, "mToolbarController", mock(DownloaderToolbarController::class.java))
        setField(fragment, "mSearchRunning", true)
        setField(fragment, "mCurrentSearch", SEARCH_TIMESTAMP)

        fragment.clearSearchQuery()
        val listener = getField(fragment, "mSearchListener") as MapSearchListener
        listener.onMapSearchResults(emptyArray(), SEARCH_TIMESTAMP, true)

        verify(adapter, never()).setSearchResultsMode(any(), any())
    }

    private fun setField(fragment: DownloaderFragment, name: String, value: Any) {
        val field = DownloaderFragment::class.java.getDeclaredField(name)
        field.isAccessible = true
        field.set(fragment, value)
    }

    private fun getField(fragment: DownloaderFragment, name: String): Any? {
        val field = DownloaderFragment::class.java.getDeclaredField(name)
        field.isAccessible = true
        return field.get(fragment)
    }

    private companion object {
        const val SEARCH_TIMESTAMP = 42L
    }
}
