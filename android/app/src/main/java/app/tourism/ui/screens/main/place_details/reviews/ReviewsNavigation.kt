package app.tourism.ui.screens.main.place_details.reviews

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.toRoute
import kotlinx.serialization.Serializable

@Serializable
object Reviews

@Serializable
object AllReviews

@Serializable
data class ReviewPics(val urls: List<String>)

@Composable
fun ReviewsNavigation(
    placeId: Long,
    rating: Double?,
    onImageClick: (selectedImage: String, imageUrls: List<String>) -> Unit,
    reviewsVM: ReviewsViewModel = hiltViewModel(),
) {
    val navController = rememberNavController()

    val onBackClick: () -> Unit = { navController.navigateUp() }
    val onMoreClick: (picsUrls: List<String>) -> Unit = {
        navController.navigate(ReviewPics(urls = it))
    }

    LaunchedEffect(Unit) {
        reviewsVM.getReviews(placeId)
    }

    NavHost(navController = navController, startDestination = Reviews) {
        composable<Reviews> {
            ReviewsScreen(
                placeId,
                rating,
                onImageClick = onImageClick,
                onSeeAllClick = {
                    navController.navigate(AllReviews)
                },
                onMoreClick = onMoreClick,
                reviewsVM = reviewsVM,
            )
        }
        composable<AllReviews> {
            AllReviewsScreen(
                reviewsVM = reviewsVM,
                onImageClick = onImageClick,
                onBackClick = onBackClick,
                onMoreClick = onMoreClick
            )
        }
        composable<ReviewPics> { navBackStackEntry ->
            val reviewPics = navBackStackEntry.toRoute<ReviewPics>()
            ReviewPicsScreen(
                urls = reviewPics.urls,
                onImageClick = onImageClick,
                onBackClick = onBackClick
            )
        }
    }
}