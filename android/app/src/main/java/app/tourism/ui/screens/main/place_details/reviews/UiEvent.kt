package app.tourism.ui.screens.main.place_details.reviews

sealed interface UiEvent {
    data object CloseReviewBottomSheet : UiEvent
    data class ShowToast(val message: String) : UiEvent
}