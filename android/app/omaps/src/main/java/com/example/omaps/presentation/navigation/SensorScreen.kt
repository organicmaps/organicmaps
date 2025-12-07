package com.example.omaps.presentation.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowUpward
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.lifecycle.viewmodel.compose.viewModel

@Composable
fun SensorScreen(
    onCancelClick: () -> Unit,
) {
    val viewModel: SensorViewModel = viewModel()
    val deviceRotation by viewModel.heading.collectAsState()
    NavigationScreen(
        distanceToNextTurn = "100 m",
        turnIcon = Icons.Default.ArrowUpward,
        remainingTime = "5 min",
        onCancelClick = onCancelClick,
        deviceRotation = deviceRotation
    )
}
