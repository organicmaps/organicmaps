package app.tourism.ui.screens.main.favorites.favorites

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.ListItem
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.SpaceForNavBar
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.SearchTopBar
import app.tourism.ui.common.nav.TopBarActionData
import app.tourism.ui.common.special.PlacesItem
import app.tourism.ui.common.ui_state.EmptyList
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun FavoritesScreen(
    onPlaceClick: (id: Long) -> Unit,
    favoritesVM: FavoritesViewModel = hiltViewModel()
) {
    val scope = rememberCoroutineScope()

    var isSearchActive by remember { mutableStateOf(false) }
    val focusRequester = remember { FocusRequester() }

    val query = favoritesVM.query.collectAsState().value
    val places = favoritesVM.places.collectAsState().value

    Scaffold(
        topBar = {
            if (isSearchActive) {
                Column {
                    SearchTopBar(
                        modifier = Modifier.focusRequester(focusRequester),
                        query = query,
                        onQueryChanged = { favoritesVM.setQuery(it) },
                        onClearClicked = { favoritesVM.clearSearchField() },
                        onBackClicked = {
                            isSearchActive = false
                            favoritesVM.setQuery("")
                        },
                    )
                }
            } else {
                AppTopBar(
                    title = stringResource(id = R.string.favorites),
                    actions = listOf(
                        TopBarActionData(
                            iconDrawable = R.drawable.search,
                            onClick = {
                                isSearchActive = true
                                scope.launch(context = Dispatchers.Main) {
                                    /*This delay is here so our textfield would first become enabled for editing
                                    and only then it should get receive focus*/
                                    delay(100L)
                                    focusRequester.requestFocus()
                                }
                            }
                        ),
                    ),
                )
            }
        },
        contentWindowInsets = Constants.USUAL_WINDOW_INSETS
    ) { paddingValues ->
        LazyColumn(Modifier.padding(paddingValues)) {
            item {
                VerticalSpace(16.dp)
            }

            if (places.isNotEmpty())
                items(places, key = { it.id }) { item ->
                    Column(Modifier.animateItem()) {
                        PlacesItem(
                            place = item,
                            onPlaceClick = { onPlaceClick(item.id) },
                            isFavorite = item.isFavorite,
                            onFavoriteChanged = { isFavorite ->
                                favoritesVM.setFavoriteChanged(item, isFavorite)
                            },
                        )
                        VerticalSpace(height = 16.dp)
                    }
                }
            else
                item {
                    EmptyList()
                }

            item {
                Column {
                    SpaceForNavBar()
                }
            }
        }
    }
}
