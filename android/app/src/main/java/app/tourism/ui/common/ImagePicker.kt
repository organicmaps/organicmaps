package app.tourism.ui.common

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage

@Composable
fun ImagePicker(
    modifier: Modifier = Modifier,
    onSuccess: ((Uri?) -> Unit)? = null,
    showPreview: Boolean = true,
    previewContentScale: ContentScale = ContentScale.Fit,
    content: @Composable () -> Unit,
) {
    var hasImage by remember {
        mutableStateOf(false)
    }
    var imageUri by rememberSaveable {
        mutableStateOf<Uri?>(null)
    }

    val imagePicker = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent(),
        onResult = { uri ->
            hasImage = uri != null
            imageUri = uri
            if (uri != null) {
                onSuccess?.invoke(imageUri)
            }
        }
    )
    Column(
        modifier = Modifier.then(modifier),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        if (showPreview && hasImage && imageUri != null) {
            AsyncImage(
                model = imageUri,
                contentScale = previewContentScale,
                contentDescription = null,
            )
            VerticalSpace(height = 10.dp)
        }

        Row(
            Modifier.clickable {
                hasImage = false
                imageUri = null
                imagePicker.launch("image/*")
            },
        ) {
            content()
        }
    }
}
