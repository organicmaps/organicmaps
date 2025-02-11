package app.tourism.ui.screens.main.home

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.repeatOnLifecycle
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.domain.models.common.PlaceShort
import app.tourism.domain.models.resource.Resource
import app.tourism.drawOverlayForTextBehind
import app.tourism.ui.ObserveAsEvents
import app.tourism.ui.common.AppSearchBar
import app.tourism.ui.common.BorderedItem
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.LoadImg
import app.tourism.ui.common.SpaceForNavBar
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.nav.AppTopBar
import app.tourism.ui.common.nav.TopBarActionData
import app.tourism.ui.common.ui_state.Error
import app.tourism.ui.common.ui_state.Loading
import app.tourism.ui.screens.main.categories.categories.CategoriesViewModel
import app.tourism.ui.screens.main.categories.categories.HorizontalSingleChoice
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getStarColor
import app.tourism.ui.utils.showToast
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext

@Composable
fun HomeScreen(
    onSearchClick: (String) -> Unit,
    onPlaceClick: (id: Long) -> Unit,
    onMapClick: () -> Unit,
    onCategoryClicked: () -> Unit,
    homeVM: HomeViewModel = hiltViewModel(),
    categoriesVM: CategoriesViewModel,
) {
    val context = LocalContext.current

    val query = homeVM.query.collectAsState().value
    val sights = homeVM.sights.collectAsState().value
    val restaurants = homeVM.restaurants.collectAsState().value

    val downloadResponse = homeVM.downloadResponse.collectAsState().value

    ObserveAsEvents(flow = homeVM.uiEventsChannelFlow) { event ->
        when (event) {
            is UiEvent.ShowToast -> context.showToast(event.message)
        }
    }

    LaunchedEffect(true) {
        categoriesVM.setSelectedCategory(null)
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
        contentWindowInsets = WindowInsets(left = 0.dp, right = 0.dp, top = 0.dp, bottom = 0.dp)
    ) { paddingValues ->
        if (downloadResponse is Resource.Success || downloadResponse is Resource.Idle)
            Column(
                Modifier
                    .padding(paddingValues)
                    .verticalScroll(rememberScrollState())
            ) {
                StartImagesDownloadIfNecessary(homeVM)

                Column(Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
                    VerticalSpace(height = 16.dp)

                    AppSearchBar(
                        modifier = Modifier.fillMaxWidth(),
                        query = query,
                        onQueryChanged = { homeVM.setQuery(it) },
                        onSearchClicked = onSearchClick,
                        onClearClicked = { homeVM.clearSearchField() },
                    )
                }
                VerticalSpace(height = 16.dp)

                Categories(categoriesVM, onCategoryClicked)
                VerticalSpace(height = 24.dp)

                HorizontalPlaces(
                    title = stringResource(id = R.string.sights),
                    items = sights,
                    onPlaceClick = { item ->
                        onPlaceClick(item.id)
                    },
                    setFavoriteChanged = { item, isFavorite ->
                        homeVM.setFavoriteChanged(item, isFavorite)
                    },
                )
                VerticalSpace(height = 24.dp)

                HorizontalPlaces(
                    title = stringResource(id = R.string.restaurants),
                    items = restaurants,
                    onPlaceClick = { item ->
                        onPlaceClick(item.id)
                    },
                    setFavoriteChanged = { item, isFavorite ->
                        homeVM.setFavoriteChanged(item, isFavorite)
                    },
                )

                SpaceForNavBar()
            }
        if (downloadResponse is Resource.Loading) {
            Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(text = stringResource(id = R.string.plz_wait_dowloading))
                    VerticalSpace(height = 16.dp)
                    Loading(onEntireScreen = false)
                }
            }
        }
        if (downloadResponse is Resource.Error) {
            Error(
                errorMessage = downloadResponse.message
                    ?: stringResource(id = R.string.smth_went_wrong),
            )
        }
    }
}

@Composable
private fun Categories(categoriesVM: CategoriesViewModel, onCategoryClicked: () -> Unit) {
    categoriesVM.apply {
        val categories = categories.collectAsState().value
        val selectedCategory = selectedCategory.collectAsState().value

        Row(
            Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState())
        ) {
            HorizontalSpace(width = 16.dp)
            BorderedItem(
                label = stringResource(id = R.string.top30),
                highlighted = true,
                onClick = { /*Nothing... Yes! Nothing!*/ },
            )
            HorizontalSpace(width = 12.dp)

            HorizontalSingleChoice(
                items = categories,
                selected = selectedCategory,
                onSelectedChanged = {
                    setSelectedCategory(it)
                    onCategoryClicked()
                },
            )
        }
    }
}

@Composable
private fun HorizontalPlaces(
    modifier: Modifier = Modifier,
    title: String,
    items: List<PlaceShort>,
    onPlaceClick: (PlaceShort) -> Unit,
    setFavoriteChanged: (PlaceShort, Boolean) -> Unit,
) {
    Column(Modifier.then(modifier)) {
        Column(Modifier.padding(horizontal = Constants.SCREEN_PADDING)) {
            Text(text = title, style = TextStyles.h2)
            VerticalSpace(height = 12.dp)
        }
        LazyRow(contentPadding = PaddingValues(horizontal = 16.dp)) {
            items(items) {
                Row {
                    Place(
                        place = it,
                        onPlaceClick = { onPlaceClick(it) },
                        isFavorite = it.isFavorite,
                        onFavoriteChanged = { isFavorite ->
                            setFavoriteChanged(it, isFavorite)
                        },
                    )
                    HorizontalSpace(width = 12.dp)
                }
            }
        }
    }
}

@Composable
private fun Place(
    modifier: Modifier = Modifier,
    place: PlaceShort,
    onPlaceClick: () -> Unit,
    isFavorite: Boolean,
    onFavoriteChanged: (Boolean) -> Unit
) {
    val textStyle = TextStyle(
        fontSize = 15.sp,
        fontWeight = FontWeight.W600,
        color = Color.White
    )

    Box(
        modifier = Modifier
            .width(230.dp)
            .height(250.dp)
            .clip(RoundedCornerShape(16.dp))
            .clickable { onPlaceClick() }
            .then(modifier),
    ) {
        LoadImg(url = place.cover)

        Column(
            Modifier
                .fillMaxWidth()
                .drawOverlayForTextBehind()
                .align(Alignment.BottomCenter)
                .padding(12.dp),
        ) {
            Text(
                text = place.name,
                style = textStyle,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis,
            )
            VerticalSpace(height = 4.dp)
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(text = "%.1f".format(place.rating), style = textStyle)
                HorizontalSpace(width = 2.dp)
                Icon(
                    modifier = Modifier.size(12.dp),
                    painter = painterResource(id = R.drawable.star),
                    contentDescription = null,
                    tint = getStarColor(),
                )
            }
        }

        IconButton(
            modifier = Modifier
                .padding(12.dp)
                .background(Color.White.copy(alpha = 0.2f), CircleShape)
                .align(Alignment.TopEnd),
            onClick = {
                onFavoriteChanged(!isFavorite)
            },
        ) {
            Icon(
                modifier = Modifier.size(20.dp),
                painter = painterResource(id = if (isFavorite) R.drawable.heart_selected else R.drawable.heart),
                contentDescription = stringResource(id = R.string.add_to_favorites),
                tint = Color.White,
            )
        }
    }
}

@Composable
fun StartImagesDownloadIfNecessary(homeVM: HomeViewModel) {
    val lifecycleOwner = LocalLifecycleOwner.current
    LaunchedEffect(Unit, lifecycleOwner) {
        // this delay is here because it might navigate to map to download it
        delay(3000L)
        lifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
            withContext(Dispatchers.Main.immediate) {
                homeVM.startDownloadServiceIfNecessary()
            }
        }
    }
}