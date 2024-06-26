package app.tourism.ui.screens.main.place_details.gallery

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import kotlinx.serialization.Serializable

@Serializable
object Gallery

@Serializable
object AllGallery

@Composable
fun GalleryNavigation(urls: List<String>) {
    val navController = rememberNavController()

    NavHost(navController = navController, startDestination = Gallery) {
        composable<Gallery> {
            GalleryScreen(
                urls = urls,
                onMoreClick = {
                    navController.navigate(AllGallery)
                },
            )
        }
        composable<AllGallery> {
            AllGalleryScreen(
                urls = urls,
                onBackClick = {
                    navController.navigateUp()
                },
            )
        }
    }
}
