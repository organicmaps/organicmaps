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
import androidx.compose.runtime.LaunchedEffect
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
    onPlaceImageClick: (selectedImage: String, imageUrls: List<String>) -> Unit,
    onBackClick: () -> Unit,
    onMapClick: () -> Unit,
    onCreateRoute: (PlaceLocation) -> Unit,
    placeVM: PlaceViewModel = hiltViewModel()
) {
    val scope = rememberCoroutineScope()

    val place = placeVM.place.collectAsState().value

    LaunchedEffect(true) {
        placeVM.observePlace(id)
    }

    Scaffold(
        topBar = {
            place?.let {
                PlaceTopBar(
                    title = it.name,
                    picUrl = it.cover,
                    isFavorite = it.isFavorite,
                    onFavoriteChanged = { isFavorite ->
                        placeVM.setFavoriteChanged(
                            id,
                            isFavorite
                        )
                    },
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
                Box(modifier = Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
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
                                    place.placeLocation?.let { it1 -> onCreateRoute(it1) }
                                },
                            )
                        }

                        1 -> {
                            GalleryNavigation(
                                urls = place.pics,
                                onItemClick = { item ->
                                    onPlaceImageClick(item, place.pics)
                                },
                            )
                        }

                        2 -> {
                            ReviewsNavigation(
                                placeId = place.id, rating = place.rating,
                                onImageClick = { selectedImageUrl, imageUrls ->
                                    onPlaceImageClick(selectedImageUrl, imageUrls)
                                },
                            )
                        }
                    }
                }
            }
        }
    }
}

