package app.organicmaps.wear

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow

data class NavigationState(
    val distToTurn: String = "",
    val nextStreet: String = "",
    val carDirection: Int = 0,
    val exitNum: Int = 0,
    val isActive: Boolean = false
)

object NavigationStateHolder {
    private val _state = MutableStateFlow(NavigationState())
    val state = _state.asStateFlow()

    fun update(newState: NavigationState) {
        _state.value = newState
    }
}
