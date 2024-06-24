package app.tourism.ui.screens.main

import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemColors
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import app.organicmaps.R
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.theme.TextStyles

@Composable
fun MainSection(themeVM: ThemeViewModel) {
    val rootNavController = rememberNavController()
    val navBackStackEntry by rootNavController.currentBackStackEntryAsState()
    val items = getNavItems()
    Scaffold { padding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
        ) {
            MainNavigation(rootNavController = rootNavController, themeVM = themeVM)

            Column(modifier = Modifier.align(alignment = Alignment.BottomCenter)) {
                NavigationBar(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp)
                        .clip(shape = RoundedCornerShape(50.dp)),
                    containerColor = MaterialTheme.colorScheme.primary,
                    windowInsets = WindowInsets(
                        left = 24.dp,
                        right = 24.dp,
                        bottom = 0.dp,
                        top = 0.dp
                    )
                ) {
                    items.forEach { item ->
                        val isSelected = item.route == navBackStackEntry?.destination?.route
                        val title = stringResource(id = item.title)
                        NavigationBarItem(
                            colors = NavigationBarItemColors(
                                disabledIconColor = MaterialTheme.colorScheme.onPrimary,
                                disabledTextColor = MaterialTheme.colorScheme.onPrimary,
                                selectedIconColor = MaterialTheme.colorScheme.onPrimary,
                                selectedTextColor = MaterialTheme.colorScheme.onPrimary,
                                unselectedIconColor = MaterialTheme.colorScheme.onPrimary,
                                unselectedTextColor = MaterialTheme.colorScheme.onPrimary,
                                selectedIndicatorColor = Color.Transparent,
                            ),
                            selected = isSelected,

                            label = {
                                Text(text = title, style = TextStyles.b3)
                            },
                            icon = {
                                Icon(
                                    painter = painterResource(
                                        if (isSelected) item.selectedIcon else item.unselectedIcon
                                    ),
                                    contentDescription = title,
                                )
                            },
                            onClick = {
                                rootNavController.navigate(item.route) {
                                    popUpTo(rootNavController.graph.findStartDestination().id) {
                                        saveState = true
                                    }
                                    launchSingleTop = true
                                    restoreState = true
                                }
                            }
                        )
                    }
                }
                VerticalSpace(height = 0.dp)
            }
        }
    }

}

data class BottomNavigationItem(
    val route: String,
    @StringRes val title: Int,
    @DrawableRes val unselectedIcon: Int,
    @DrawableRes val selectedIcon: Int
)

@Composable
fun getNavItems(): List<BottomNavigationItem> {
    return listOf(
        BottomNavigationItem(
            route = "home_tab",
            title = R.string.home,
            selectedIcon = R.drawable.home_selected,
            unselectedIcon = R.drawable.home,
        ),
        BottomNavigationItem(
            route = "categories_tab",
            title = R.string.categories,
            selectedIcon = R.drawable.categories_selected,
            unselectedIcon = R.drawable.categories,
        ),
        BottomNavigationItem(
            route = "favorites_tab",
            title = R.string.favorites,
            selectedIcon = R.drawable.heart_selected,
            unselectedIcon = R.drawable.heart,
        ),
        BottomNavigationItem(
            route = "profile_tab",
            title = R.string.profile_tourism,
            selectedIcon = R.drawable.profile_selected,
            unselectedIcon = R.drawable.profile,
        ),
    )
}
