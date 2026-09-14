package app.organicmaps.maplayer

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import app.organicmaps.sdk.location.TrackRecorder

/**
 * Start-side navigation column (landscape, tablets): [freeHeight] is the space left between the
 * maneuver card and the collapsed ETA panel, [panelHeight] is how far that panel reaches up from
 * the window bottom (peek height + navigation-bar strip), [endSlotWidth] is the width the speed
 * limit sign holds in the opposite top corner, which anything spanning the top row must clear.
 */
data class NavColumnMetrics(val freeHeight: Int = 0, val panelHeight: Int = 0, val endSlotWidth: Int = 0)

class MapButtonsViewModel : ViewModel() {

    private val _buttonsHidden = MutableLiveData(false)
    val buttonsHidden: LiveData<Boolean> = _buttonsHidden

    private val _bottomButtonsHidden = MutableLiveData(false)
    val bottomButtonsHidden: LiveData<Boolean> = _bottomButtonsHidden

    private val _fullscreen = MutableLiveData(false)
    val fullscreen: LiveData<Boolean> = _fullscreen

    private val _bottomButtonsHeight = MutableLiveData(0f)
    val bottomButtonsHeight: LiveData<Float> = _bottomButtonsHeight

    private val _topButtonsMarginTop = MutableLiveData(-1)
    val topButtonsMarginTop: LiveData<Int> = _topButtonsMarginTop

    private val _layoutMode = MutableLiveData(MapButtonsController.LayoutMode.regular)
    val layoutMode: LiveData<MapButtonsController.LayoutMode> = _layoutMode

    private val _myPositionMode = MutableLiveData<Int>()
    val myPositionMode: LiveData<Int> = _myPositionMode

    private val _searchOption = MutableLiveData<SearchOptionsButton.SearchOption?>()
    val searchOption: LiveData<SearchOptionsButton.SearchOption?> = _searchOption

    private val _trackRecorderState = MutableLiveData(TrackRecorder.nativeIsTrackRecordingEnabled())
    val trackRecorderState: LiveData<Boolean> = _trackRecorderState

    // Height of the top header (routing plan / navigation frame) the search sheet must clear when expanded.
    private val _topHeaderHeight = MutableLiveData(0)
    val topHeaderHeight: LiveData<Int> = _topHeaderHeight

    private val _navColumnMetrics = MutableLiveData(NavColumnMetrics())
    val navColumnMetrics: LiveData<NavColumnMetrics> = _navColumnMetrics

    fun setButtonsHidden(buttonsHidden: Boolean) {
        _buttonsHidden.value = buttonsHidden
    }

    fun setBottomButtonsHidden(buttonsHidden: Boolean) {
        _bottomButtonsHidden.value = buttonsHidden
    }

    fun setFullscreen(fullscreen: Boolean) {
        _fullscreen.value = fullscreen
    }

    fun setBottomButtonsHeight(height: Float) {
        _bottomButtonsHeight.value = height
    }

    fun setTopButtonsMarginTop(margin: Int) {
        _topButtonsMarginTop.value = margin
    }

    fun setLayoutMode(layoutMode: MapButtonsController.LayoutMode) {
        _layoutMode.value = layoutMode
    }

    fun setMyPositionMode(mode: Int) {
        _myPositionMode.value = mode
    }

    fun setSearchOption(searchOption: SearchOptionsButton.SearchOption?) {
        _searchOption.value = searchOption
    }

    fun setTrackRecorderState(state: Boolean) {
        _trackRecorderState.value = state
    }

    fun setTopHeaderHeight(height: Int) {
        // Layout listeners call this on every pass; skip redundant updates so the search sheet's
        // expanded offset is recomputed only when the height actually changes.
        if (_topHeaderHeight.value != height) {
            _topHeaderHeight.value = height
        }
    }

    fun setNavColumnMetrics(metrics: NavColumnMetrics) {
        // Same as above: layout listeners fire on every pass, and re-applying the placement would
        // schedule another one.
        if (_navColumnMetrics.value != metrics) {
            _navColumnMetrics.value = metrics
        }
    }
}
