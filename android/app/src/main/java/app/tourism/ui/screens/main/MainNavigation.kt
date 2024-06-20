package app.tourism.ui.screens.main

import android.content.Context
import android.content.Intent
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.toRoute
import app.tourism.AuthActivity
import app.tourism.ui.screens.auth.Language
import app.tourism.ui.screens.language.LanguageScreen
import app.tourism.ui.screens.main.categories.categories.CategoriesScreen
import app.tourism.ui.screens.main.favorites.favorites.FavoritesScreen
import app.tourism.ui.screens.main.home.home.HomeScreen
import app.tourism.ui.screens.main.profile.personal_data.PersonalDataScreen
import app.tourism.ui.screens.main.profile.profile.ProfileScreen
import app.tourism.ui.screens.main.site_details.SiteDetailsScreen
import app.tourism.utils.navigateToMap
import app.tourism.utils.navigateToMapForRoute
import kotlinx.serialization.Serializable

// tabs
@Serializable
object HomeTab

@Serializable
object CategoriesTab

@Serializable
object FavoritesTab

@Serializable
object ProfileTab

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

// site details
@Serializable
data class SiteDetails(val id: Int)

@Composable
fun MainNavigation(rootNavController: NavHostController, themeVM: ThemeViewModel) {
    val context = LocalContext.current

    val onSiteClick: (id: Int) -> Unit = { id ->
        rootNavController.navigate(SiteDetails(id = id))
    }
    val onMapClick = { navigateToMap(context) }

    NavHost(rootNavController, startDestination = HomeTab) {
        composable<HomeTab> {
            HomeNavHost(onSiteClick, onMapClick)
        }
        composable<CategoriesTab> {
            CategoriesNavHost(onSiteClick, onMapClick)
        }
        composable<FavoritesTab> {
            FavoritesNavHost(onSiteClick)
        }
        composable<ProfileTab> {
            ProfileNavHost(themeVM = themeVM)
        }
        composable<SiteDetails> { backStackEntry ->
            val siteDetails = backStackEntry.toRoute<SiteDetails>()
            SiteDetailsScreen(
                id = siteDetails.id,
                onBackClick = { rootNavController.navigateUp() },
                onMapClick = onMapClick,
                onCreateRoute = { siteLocation ->
                    navigateToMapForRoute(context, siteLocation)
                }
            )
        }
    }
}

@Composable
fun HomeNavHost(onSiteClick: (id: Int) -> Unit, onMapClick: () -> Unit) {
    val homeNavController = rememberNavController()
    NavHost(homeNavController, startDestination = Home) {
        composable<Home> {
            HomeScreen(
                onSearchClick = { query ->
                    homeNavController.navigate(Search(query = query))
                },
                onSiteClick = onSiteClick,
                onMapClick = onMapClick
            )
        }
        composable<Search> { backStackEntry ->
            val search = backStackEntry.toRoute<Search>()
            Search(query = search.query)
        }
    }
}

@Composable
fun CategoriesNavHost(onSiteClick: (id: Int) -> Unit, onMapClick: () -> Unit) {
    val categoriesNavController = rememberNavController()
    NavHost(categoriesNavController, startDestination = Categories) {
        composable<Categories> {
            CategoriesScreen(onSiteClick, onMapClick)
        }
    }
}

@Composable
fun FavoritesNavHost(onSiteClick: (id: Int) -> Unit) {
    val favoritesNavController = rememberNavController()
    NavHost(favoritesNavController, startDestination = Favorites) {
        composable<Favorites> {
            FavoritesScreen(onSiteClick)
        }
    }
}

@Composable
fun ProfileNavHost(themeVM: ThemeViewModel) {
    val context = LocalContext.current
    val profileNavController = rememberNavController()
    val onBackClick = { profileNavController.navigateUp() }

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
                themeVM = themeVM
            )
        }
        composable<PersonalData> {
            PersonalDataScreen(onBackClick)
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