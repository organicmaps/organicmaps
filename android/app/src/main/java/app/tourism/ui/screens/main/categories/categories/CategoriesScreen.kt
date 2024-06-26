package app.tourism.ui.screens.main.categories.categories

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
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
    onMapClick: () -> Unit,
    categoriesVM: CategoriesViewModel = hiltViewModel()
) {
    categoriesVM.apply {
        val query = query.collectAsState().value
        val categories = categories.collectAsState().value
        val selectedCategory = selectedCategory.collectAsState().value
        val places = places.collectAsState().value

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
            contentWindowInsets = Constants.USUAL_WINDOW_INSETS
        ) { paddingValues ->
            LazyColumn(
                Modifier
                    .padding(paddingValues),
            ) {
                item {
                    Column {
                        VerticalSpace(height = 16.dp)

                        AppSearchBar(
                            modifier = Modifier.fillMaxWidth(),
                            query = query,
                            onQueryChanged = ::setQuery,
                            onSearchClicked = ::search,
                            onClearClicked = ::clearSearchField,
                        )
                        VerticalSpace(height = 16.dp)

                        HorizontalSingleChoice(
                            items = categories,
                            selected = selectedCategory,
                            onSelectedChanged = ::setSelectedCategory,
                        )
                        VerticalSpace(height = 16.dp)
                    }
                }

                items(places) { item ->
                    Column {
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
