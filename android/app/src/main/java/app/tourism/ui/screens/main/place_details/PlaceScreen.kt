package app.tourism.ui.screens.main.place_details

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.tourism.Constants
import app.tourism.data.dto.PlaceLocation
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.nav.PlaceTopBar
import app.tourism.ui.screens.main.place_details.description.DescriptionScreen
import app.tourism.ui.screens.main.place_details.gallery.GalleryNavigation
import app.tourism.ui.screens.main.place_details.reviews.ReviewsNavigation
import kotlinx.coroutines.launch

@Composable
fun PlaceDetailsScreen(
    id: Long,
    onBackClick: () -> Unit,
    onMapClick: () -> Unit,
    onCreateRoute: (PlaceLocation) -> Unit,
    placeVM: PlaceViewModel = hiltViewModel()
) {
    val scope = rememberCoroutineScope()

    val place = placeVM.place.collectAsState().value

    Scaffold(
        topBar = {
            place?.let {
                PlaceTopBar(
                    title = it.name,
                    picUrl = it.pic,
                    isFavorite = it.isFavorite,
                    onFavoriteChanged = { placeVM.setFavoriteChanged(id, it) },
                    onMapClick = onMapClick,
                    onBackClick = onBackClick,
                )
            }
        },
        contentWindowInsets = WindowInsets.statusBars
    ) { paddingValues ->
        place?.let {
            Column(Modifier.padding(paddingValues)) {
                val pagerState = rememberPagerState(pageCount = { 3 })

                VerticalSpace(height = 16.dp)
                Box(modifier = Modifier.padding(horizontal = Constants.SCREEN_PADDING)){
                    PlaceTabRow(
                        tabIndex = pagerState.currentPage,
                        onTabIndexChanged = {
                            scope.launch {
                                pagerState.scrollToPage(it)
                            }
                        },
                    )
                }


                HorizontalPager(
                    modifier = Modifier.fillMaxSize(),
                    state = pagerState,
                    verticalAlignment = Alignment.Top,
                ) { page ->
                    when (page) {
                        0 -> {
                            DescriptionScreen(
                                description = place.description,
                                onCreateRoute = {
                                    place.placeLocation?.let { onCreateRoute(it) }
                                },
                            )
                        }

                        1 -> {
                            GalleryNavigation(urls = place.pics)
                        }

                        2 -> {
                            ReviewsNavigation(placeId = place.id, rating = place.rating)
                        }
                    }
                }
            }
        }
    }
}

