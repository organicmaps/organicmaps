package app.organicmaps.wear.presentation

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.wear.compose.foundation.pager.HorizontalPager
import androidx.wear.compose.foundation.pager.rememberPagerState
import androidx.wear.compose.material.Text
import app.organicmaps.wear.NavigationStateHolder
import app.organicmaps.wear.presentation.navigation.NavigationScreen
import app.organicmaps.wear.presentation.navigation.SensorViewModel
import app.organicmaps.wear.presentation.search.SearchScreen
import app.organicmaps.wear.presentation.theme.OrganicMapsTheme
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowUpward

class Omaps : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setTheme(android.R.style.Theme_DeviceDefault)
        setContent {
            WearApp()
        }
    }
}

@Composable
fun WearApp() {
    val navState by NavigationStateHolder.state.collectAsState()
    val isNavigating = navState.isActive
    
    val pagerState = rememberPagerState(pageCount = { if (isNavigating) 3 else 1 })

    OrganicMapsTheme {
        if (!isNavigating) {
            SearchScreen(onSearchClick = { 
                // For now, this just simulates starting navigation locally if needed, 
                // but real data comes from the phone.
            })
        } else {
            HorizontalPager(
                state = pagerState,
                modifier = Modifier.fillMaxSize()
            ) { page ->
                when (page) {
                    0 -> MapPanel()
                    1 -> NavigationPanel(navState)
                    2 -> StatsPanel()
                }
            }
        }
    }
}

@Composable
fun NavigationPanel(navState: app.organicmaps.wear.NavigationState) {
    val sensorViewModel: SensorViewModel = viewModel()
    val deviceRotation by sensorViewModel.heading.collectAsState()
    
    NavigationScreen(
        distanceToNextTurn = navState.distToTurn,
        turnIcon = Icons.Default.ArrowUpward, 
        remainingTime = "",
        onCancelClick = { /* TODO: Send stop command back to phone */ },
        deviceRotation = deviceRotation
    )
}

@Composable
fun MapPanel() {
    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Text("Map View\n(Vector Rendering)")
    }
}

@Composable
fun StatsPanel() {
    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Text("Stats View\n(Speed, Alt, etc.)")
    }
}
