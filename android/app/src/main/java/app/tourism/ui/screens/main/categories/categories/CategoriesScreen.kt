package app.tourism.ui.screens.main.categories.categories

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.AppSearchBar
import app.tourism.ui.common.SpaceForNavBar
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData
import app.tourism.ui.common.special.PlacesItem

@Composable
fun CategoriesScreen(
    onPlaceClick: (id: Long) -> Unit,
    onSearchClick: (String) -> Unit,
    onMapClick: () -> Unit,
    categoriesVM: CategoriesViewModel = hiltViewModel()
) {
    categoriesVM.apply {
        val query = query.collectAsState().value
        val categories = categories.collectAsState().value
        val selectedCategory = selectedCategory.collectAsState().value
        val places = places.collectAsState().value

        LaunchedEffect(true) {
            if (selectedCategory == null)
                categoriesVM.setSelectedCategory(categories.first())
        }

        Scaffold(
            topBar = {
                AppTopBar(
                    title = stringResource(id = R.string.categories),
                    actions = listOf(
                        TopBarActionData(
                            iconDrawable = R.drawable.map,
                            color = MaterialTheme.colorScheme.primary,
                            onClick = onMapClick
                        ),
                    ),
                )
            },
            contentWindowInsets = WindowInsets(bottom = 0.dp)
        ) { paddingValues ->
            LazyColumn(Modifier.padding(paddingValues)) {
                item {
                    Column {
                        Column(modifier = Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
                            VerticalSpace(height = 16.dp)

                            AppSearchBar(
                                modifier = Modifier.fillMaxWidth(),
                                query = query,
                                onQueryChanged = ::setQuery,
                                onSearchClicked = onSearchClick,
                                onClearClicked = ::clearSearchField,
                            )
                            VerticalSpace(height = 16.dp)
                        }
                        HorizontalSingleChoice(
                            modifier = Modifier
                                .fillMaxWidth()
                                .horizontalScroll(rememberScrollState())
                                .padding(horizontal = Constants.SCREEN_PADDING),
                            items = categories,
                            selected = selectedCategory,
                            onSelectedChanged = ::setSelectedCategory,
                        )
                        VerticalSpace(height = 16.dp)
                    }
                }

                items(places) { item ->
                    Column(modifier = Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
                        PlacesItem(
                            place = item,
                            onPlaceClick = { onPlaceClick(item.id) },
                            isFavorite = item.isFavorite,
                            onFavoriteChanged = { isFavorite ->
                                setFavoriteChanged(item, isFavorite)
                            },
                        )
                        VerticalSpace(height = 16.dp)
                    }
                }

                item {
                    Column {
                        SpaceForNavBar()
                    }
                }
            }
        }
    }
}
