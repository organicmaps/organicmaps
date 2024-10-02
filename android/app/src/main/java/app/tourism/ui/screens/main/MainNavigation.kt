package app.tourism.ui.screens.main

import FullscreenImageScreen
import android.content.Context
import android.content.Intent
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.toRoute
import app.tourism.AuthActivity
import app.tourism.ui.screens.auth.Language
import app.tourism.ui.screens.language.LanguageScreen
import app.tourism.ui.screens.main.categories.categories.CategoriesScreen
import app.tourism.ui.screens.main.categories.categories.CategoriesViewModel
import app.tourism.ui.screens.main.favorites.favorites.FavoritesScreen
import app.tourism.ui.screens.main.home.HomeScreen
import app.tourism.ui.screens.main.place_details.PlaceDetailsScreen
import app.tourism.ui.screens.main.profile.personal_data.PersonalDataScreen
import app.tourism.ui.screens.main.profile.profile.ProfileScreen
import app.tourism.ui.screens.main.profile.profile.ProfileViewModel
import app.tourism.ui.screens.main.search.SearchScreen
import app.tourism.utils.navigateToMap
import app.tourism.utils.navigateToMapForRoute
import kotlinx.serialization.Serializable

// home
@Serializable
object Home

@Serializable
data class Search(val query: String)

// categories
@Serializable
object Categories

// favorites
@Serializable
object Favorites

// profile
@Serializable
object Profile

@Serializable
object PersonalData

// place details
@Serializable
data class PlaceDetails(val id: Long)

@Serializable
data class FullscreenImageViewer(val selectedImageUrl: String, val imageUrls: List<String>)

@Composable
fun MainNavigation(rootNavController: NavHostController, themeVM: ThemeViewModel) {
    val context = LocalContext.current

    val categoriesVM: CategoriesViewModel = hiltViewModel()

    val onPlaceClick: (id: Long) -> Unit = { id ->
        rootNavController.navigate(PlaceDetails(id = id))
    }
    val onPlaceImageClick: (String, List<String>) -> Unit = { selectedImage, imageUrls ->
        rootNavController.navigate(FullscreenImageViewer(selectedImage, imageUrls))
    }
    val onSearchClick: (q: String) -> Unit = { q ->
        rootNavController.navigate(Search(query = q))
    }
    val onMapClick = { navigateToMap(context) }
    val onBackClick: () -> Unit = { rootNavController.navigateUp() }

    NavHost(rootNavController, startDestination = "home_tab") {
        composable("home_tab") {
            HomeNavHost(
                onPlaceClick,
                onSearchClick,
                onMapClick,
                onCategoryClicked = {
                    rootNavController.navigate("categories_tab") {
                        popUpTo(rootNavController.graph.findStartDestination().id) {
                            saveState = true
                        }
                        launchSingleTop = true
                        restoreState = true
                    }
                },
                categoriesVM,
            )
        }
        composable("categories_tab") {
            CategoriesNavHost(onPlaceClick, onSearchClick, onMapClick, categoriesVM)
        }
        composable("favorites_tab") {
            FavoritesNavHost(onPlaceClick)
        }
        composable("profile_tab") {
            ProfileNavHost(themeVM = themeVM)
        }
        composable<PlaceDetails> { backStackEntry ->
            val placeDetails = backStackEntry.toRoute<PlaceDetails>()
            PlaceDetailsScreen(
                id = placeDetails.id,
                onPlaceImageClick = onPlaceImageClick,
                onBackClick = onBackClick,
                onMapClick = onMapClick,
                onCreateRoute = { placeLocation ->
                    navigateToMapForRoute(context, placeLocation)
                }
            )
        }
        composable<FullscreenImageViewer> { backStackEntry ->
            val fullscreenImageViewer = backStackEntry.toRoute<FullscreenImageViewer>()
            FullscreenImageScreen(
                onBackClick = onBackClick,
                selectedImageUrl = fullscreenImageViewer.selectedImageUrl,
                imageUrls = fullscreenImageViewer.imageUrls
            )
        }
        composable<Search> { backStackEntry ->
            val search = backStackEntry.toRoute<Search>()
            SearchScreen(
                onPlaceClick = onPlaceClick,
                onBackClick = onBackClick,
                onMapClick = onMapClick,
                queryArg = search.query,
            )
        }
    }
}

@Composable
fun HomeNavHost(
    onPlaceClick: (id: Long) -> Unit,
    onSearchClick: (String) -> Unit,
    onMapClick: () -> Unit,
    onCategoryClicked: () -> Unit,
    categoriesVM: CategoriesViewModel,
) {
    val homeNavController = rememberNavController()
    NavHost(homeNavController, startDestination = Home) {
        composable<Home> {
            HomeScreen(
                onSearchClick = onSearchClick,
                onPlaceClick = onPlaceClick,
                onMapClick = onMapClick,
                onCategoryClicked = onCategoryClicked,
                categoriesVM = categoriesVM
            )
        }
    }
}

@Composable
fun CategoriesNavHost(
    onPlaceClick: (id: Long) -> Unit,
    onSearchClick: (String) -> Unit,
    onMapClick: () -> Unit,
    categoriesVM: CategoriesViewModel,
) {
    val categoriesNavController = rememberNavController()
    NavHost(categoriesNavController, startDestination = Categories) {
        composable<Categories> {
            CategoriesScreen(onPlaceClick, onSearchClick, onMapClick, categoriesVM)
        }
    }
}

@Composable
fun FavoritesNavHost(onPlaceClick: (id: Long) -> Unit) {
    val favoritesNavController = rememberNavController()
    NavHost(favoritesNavController, startDestination = Favorites) {
        composable<Favorites> {
            FavoritesScreen(onPlaceClick)
        }
    }
}

@Composable
fun ProfileNavHost(themeVM: ThemeViewModel, profileVM: ProfileViewModel = hiltViewModel()) {
    val context = LocalContext.current
    val profileNavController = rememberNavController()
    val onBackClick: () -> Unit = { profileNavController.navigateUp() }

    NavHost(profileNavController, startDestination = Profile) {
        composable<Profile> {
            ProfileScreen(
                onPersonalDataClick = {
                    profileNavController.navigate(PersonalData)
                },
                onLanguageClick = {
                    profileNavController.navigate(Language)
                },
                onSignOutComplete = {
                    navigateToAuth(context)
                },
                profileVM = profileVM,
                themeVM = themeVM
            )
        }
        composable<PersonalData> {
            PersonalDataScreen(onBackClick, profileVM)
        }
        composable<Language> {
            LanguageScreen(onBackClick)
        }
    }
}

private fun navigateToAuth(context: Context) {
    val intent = Intent(context, AuthActivity::class.java)
    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK or Intent.FLAG_ACTIVITY_NEW_TASK)
    ContextCompat.startActivity(context, intent, null)
}