package app.tourism.ui.screens.main.place_details.reviews

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import app.tourism.Constants
import app.tourism.ui.common.LoadImg
import app.tourism.ui.common.nav.BackButtonWithText
import app.tourism.ui.screens.main.place_details.gallery.propertiesForSmallImage

@Composable
fun ReviewPicsScreen(
    urls: List<String>,
    onImageClick: (selectedImage: String, imageUrls: List<String>) -> Unit,
    onBackClick: () -> Unit
) {
    Scaffold(
        topBar = {
            BackButtonWithText { onBackClick() }
        }
    ) { paddingValues ->
        LazyVerticalGrid(
            modifier = Modifier.padding(paddingValues),
            columns = GridCells.Fixed(2),
            contentPadding = PaddingValues(Constants.SCREEN_PADDING),
            horizontalArrangement = Arrangement.spacedBy(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            items(urls) {
                LoadImg(
                    modifier = Modifier
                        .clickable { onImageClick(it, urls) }
                        .propertiesForSmallImage(), url = it
                )
            }
        }
    }
}