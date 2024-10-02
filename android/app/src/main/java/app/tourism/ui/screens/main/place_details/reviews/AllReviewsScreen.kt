package app.tourism.ui.screens.main.place_details.reviews

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Modifier
import androidx.hilt.navigation.compose.hiltViewModel
import app.tourism.Constants
import app.tourism.ui.common.nav.BackButtonWithText
import app.tourism.ui.screens.main.place_details.reviews.components.Review

@Composable
fun AllReviewsScreen(
    reviewsVM: ReviewsViewModel = hiltViewModel(),
    onImageClick: (selectedImage: String, imageUrls: List<String>) -> Unit,
    onBackClick: () -> Unit,
    onMoreClick: (picsUrls: List<String>) -> Unit,
) {
    val reviews = reviewsVM.reviews.collectAsState().value

    Scaffold(
        topBar = {
            BackButtonWithText { onBackClick() }
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier.padding(paddingValues),
            contentPadding = PaddingValues(Constants.SCREEN_PADDING),
        ) {
            items(reviews) {
                Review(review = it, onMoreClick = onMoreClick, onImageClick = onImageClick)
            }
        }
    }
}
