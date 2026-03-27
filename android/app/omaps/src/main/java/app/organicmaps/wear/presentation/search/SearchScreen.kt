package app.organicmaps.wear.presentation.search

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Search
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.wear.compose.material.Chip
import androidx.wear.compose.material.Icon
import androidx.wear.compose.material.Scaffold
import androidx.wear.compose.material.ScalingLazyColumn
import androidx.wear.compose.material.Text
import androidx.wear.compose.material.TimeText
import androidx.wear.compose.material.rememberScalingLazyListState

@Composable
fun SearchScreen(onSearchClick: () -> Unit) {
    Scaffold(timeText = { TimeText() }) {
        val listState = rememberScalingLazyListState()
        ScalingLazyColumn(
            modifier = Modifier.fillMaxSize(),
            state = listState,
            contentPadding = PaddingValues(
                top = 32.dp,
                start = 8.dp,
                end = 8.dp,
                bottom = 32.dp,
            ),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
        ) {
            item {
                Chip(
                    onClick = onSearchClick,
                    label = { Text("Search") },
                    icon = { Icon(Icons.Default.Search, contentDescription = "Search icon") },
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
    }
}
