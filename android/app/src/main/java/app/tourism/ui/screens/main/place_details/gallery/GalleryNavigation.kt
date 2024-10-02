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
fun GalleryNavigation(urls: List<String>, onItemClick: (String) -> Unit) {
    val navController = rememberNavController()

    NavHost(navController = navController, startDestination = Gallery) {
        composable<Gallery> {
            GalleryScreen(
                urls = urls,
                onItemClick = onItemClick,
                onMoreClick = {
                    navController.navigate(AllGallery)
                },
            )
        }
        composable<AllGallery> {
            AllGalleryScreen(
                urls = urls,
                onItemClick = onItemClick,
                onBackClick = {
                    navController.navigateUp()
                },
            )
        }
    }
}
