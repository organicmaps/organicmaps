package app.tourism.ui.screens.main.place_details.reviews

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.special.RatingBar
import app.tourism.ui.screens.main.place_details.reviews.components.PostReview
import app.tourism.ui.screens.main.place_details.reviews.components.Review
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getStarColor
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlin.math.roundToInt

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ReviewsScreen(
    placeId: Long,
    rating: Double?,
    onSeeAllClick: () -> Unit,
    onMoreClick: (picsUrls: List<String>) -> Unit,
    reviewsVM: ReviewsViewModel = hiltViewModel(),
) {
    val scope = rememberCoroutineScope()

    val sheetState = rememberModalBottomSheetState()
    var showReviewBottomSheet by remember { mutableStateOf(false) }

    val userReview = reviewsVM.userReview.collectAsState().value
    val reviews = reviewsVM.reviews.collectAsState().value

    LazyColumn(
        contentPadding = PaddingValues(Constants.SCREEN_PADDING),
    ) {
        rating?.let {
            item {
                Column {
                    VerticalSpace(height = 16.dp)
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            modifier = Modifier.size(30.dp),
                            painter = painterResource(id = R.drawable.star),
                            contentDescription = null,
                            tint = getStarColor(),
                        )
                        HorizontalSpace(width = 8.dp)
                        Text(text = "%.1f".format(rating) + "/5", style = TextStyles.h1)
                    }

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        TextButton(
                            onClick = {
                                showReviewBottomSheet = true
                                scope.launch {
                                    // Have to do add this delay, because bottom sheet doesn't expand fully itself
                                    // and expands with duration after showReviewBottomSheet is set to true
                                    delay(300L)
                                    sheetState.expand()
                                }
                            },
                        ) {
                            Text(text = stringResource(id = R.string.compose_review))
                        }

                        RatingBar(rating = it.roundToInt())
                    }
                    VerticalSpace(height = 24.dp)

                    TextButton(
                        modifier = Modifier.align(Alignment.End),
                        onClick = {
                            onSeeAllClick()
                        },
                    ) {
                        Text(text = stringResource(id = R.string.see_all))
                    }
                }
            }
        }

        userReview?.let {
            item {
                Review(
                    review = userReview,
                    onMoreClick = onMoreClick,
                    onDeleteClick = {}
                )
            }
        }

        if (reviews.firstOrNull() != null)
            item {
                Review(review = reviews[0], onMoreClick = onMoreClick)
            }
    }

    if (showReviewBottomSheet)
        ModalBottomSheet(
            sheetState = sheetState,
            containerColor = MaterialTheme.colorScheme.background,
            onDismissRequest = { showReviewBottomSheet = false },
        ) {
            PostReview(
                placeId,
                onPostReviewSuccess = {
                    showReviewBottomSheet = false
                },
            )
        }
}
