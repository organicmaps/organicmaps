package app.tourism.ui.screens.main.home.search

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
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
import app.tourism.ui.theme.TextStyles

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun SearchScreen(
    onPlaceClick: (id: Int) -> Unit,
    onMapClick: () -> Unit,
    queryArg: String,
    searchVM: SearchViewModel = hiltViewModel()
) {
    val query = searchVM.query.collectAsState().value
    val places = searchVM.places.collectAsState().value
    val itemsNumber = searchVM.itemsNumber.collectAsState().value

    LaunchedEffect(Unit) {
        searchVM.setQuery(queryArg)
    }

    Scaffold(
        topBar = {
            AppTopBar(
                title = stringResource(id = R.string.tjk),
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
        LazyColumn(Modifier.padding(paddingValues)) {
            stickyHeader {
                Column {
                    VerticalSpace(height = 16.dp)

                    AppSearchBar(
                        modifier = Modifier.fillMaxWidth(),
                        query = query,
                        onQueryChanged = { searchVM.setQuery(it) },
                        onSearchClicked = { searchVM.search(it) },
                        onClearClicked = { searchVM.clearSearchField() },
                    )
                    VerticalSpace(height = 16.dp)
                }
            }

            item {
                itemsNumber?.let {
                    Column {
                        Text(
                            text = "${stringResource(id = R.string.found)} $it",
                            style = TextStyles.h3,
                        )
                        VerticalSpace(height = 16.dp)
                    }
                }
            }

            items(places) { item ->
                Column {
                    PlacesItem(
                        place = item,
                        onPlaceClick = { onPlaceClick(item.id) },
                        isFavorite = item.isFavorite,
                        onFavoriteChanged = { isFavorite ->
                            searchVM.setFavoriteChanged(item, isFavorite)
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