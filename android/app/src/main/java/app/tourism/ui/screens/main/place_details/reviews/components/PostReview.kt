package app.tourism.ui.screens.main.place_details.reviews.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import app.organicmaps.R
import app.tourism.Constants
import app.tourism.ui.common.ImagePicker
import app.tourism.ui.common.VerticalSpace
import app.tourism.ui.common.buttons.PrimaryButton
import app.tourism.ui.common.special.RatingBar
import app.tourism.ui.common.textfields.AppEditText
import app.tourism.ui.screens.main.place_details.reviews.PostReviewViewModel
import app.tourism.ui.theme.TextStyles
import app.tourism.ui.theme.getBorderColor
import app.tourism.utils.FileUtils
import coil.compose.AsyncImage
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.File

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun PostReview(
    modifier: Modifier = Modifier,
    postReviewVM: PostReviewViewModel = hiltViewModel(),
) {
    val scope = rememberCoroutineScope()
    val context = LocalContext.current

    val rating = postReviewVM.rating.collectAsState().value
    val comment = postReviewVM.comment.collectAsState().value
    val files = postReviewVM.files.collectAsState().value

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = Constants.SCREEN_PADDING)
            .then(modifier),
    ) {
        Text(text = stringResource(id = R.string.review_title), style = TextStyles.h2)
        VerticalSpace(height = 32.dp)

        Column(
            Modifier.fillMaxWidth(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(text = stringResource(id = R.string.tap_to_rate), style = TextStyles.b3)
            VerticalSpace(height = 4.dp)
            RatingBar(rating = rating, onRatingChanged = { postReviewVM.setRating(it) })
        }
        VerticalSpace(height = 16.dp)

        AppEditText(
            value = comment, onValueChange = { postReviewVM.setComment(it) },
            hint = stringResource(id = R.string.text),
            maxLines = 10
        )
        VerticalSpace(height = 32.dp)

        FlowRow(
            horizontalArrangement = Arrangement.spacedBy(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            files.forEach {
                ImagePreview(
                    model = it,
                    onDelete = {
                        postReviewVM.removeFile(it)
                    },
                )
            }
            ImagePicker(
                showPreview = false,
                onSuccess = { uri ->
                    scope.launch(Dispatchers.IO) {
                        postReviewVM.addFile(
                            File(FileUtils(context).getPath(uri))
                        )
                    }
                }
            ) {
                AddPhoto()
            }
        }
        VerticalSpace(height = 32.dp)

        PrimaryButton(
            label = stringResource(id = R.string.send),
            onClick = { postReviewVM.postReview() },
        )
        VerticalSpace(height = 64.dp)
    }
}

@Composable
fun AddPhoto() {
    Box(
        modifier = Modifier
            .getPhotoBoxProperties()
            .background(color = MaterialTheme.colorScheme.surface),
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Icon(
                painter = painterResource(id = R.drawable.add),
                contentDescription = null,
                tint = MaterialTheme.colorScheme.onBackground
            )
            VerticalSpace(height = 8.dp)
            Text(
                text = stringResource(id = R.string.upload_photo),
                style = TextStyles.b2,
                color = MaterialTheme.colorScheme.onBackground
            )
        }
    }
}

@Composable
fun ImagePreview(model: Any?, onDelete: () -> Unit) {
    Box {
        AsyncImage(
            modifier = Modifier
                .getPhotoBoxProperties(),
            model = model,
            contentScale = ContentScale.Crop,
            contentDescription = null
        )

        Icon(
            modifier = Modifier
                .size(30.dp)
                .align(alignment = Alignment.TopEnd)
                .clickable { onDelete() }
                .offset(x = 12.dp, y = (-12).dp),
            painter = painterResource(id = R.drawable.ic_route_remove),
            contentDescription = null,
            tint = MaterialTheme.colorScheme.primary
        )
    }
}

@Composable
fun Modifier.getPhotoBoxProperties() =
    this
        .width(104.dp)
        .aspectRatio(1f)
        .border(
            width = 1.dp,
            color = getBorderColor(),
            shape = photoShape
        )
        .clip(photoShape)

val photoShape = RoundedCornerShape(12.dp)
