package app.organicmaps.maplayer

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import app.organicmaps.sdk.location.TrackRecorder

class MapButtonsViewModel : ViewModel() {

    private val _buttonsHidden = MutableLiveData(false)
    val buttonsHidden: LiveData<Boolean> = _buttonsHidden

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

    private val _searchOption = MutableLiveData<SearchWheel.SearchOption?>()
    val searchOption: LiveData<SearchWheel.SearchOption?> = _searchOption

    private val _trackRecorderState = MutableLiveData(TrackRecorder.nativeIsTrackRecordingEnabled())
    val trackRecorderState: LiveData<Boolean> = _trackRecorderState

    fun setButtonsHidden(buttonsHidden: Boolean) {
        _buttonsHidden.value = buttonsHidden
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

    fun setSearchOption(searchOption: SearchWheel.SearchOption?) {
        _searchOption.value = searchOption
    }

    fun setTrackRecorderState(state: Boolean) {
        _trackRecorderState.value = state
    }
}
