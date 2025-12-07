package com.example.omaps.presentation.navigation

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowUpward
import androidx.compose.material.icons.filled.Close
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.wear.compose.material.Button
import androidx.wear.compose.material.Icon
import androidx.wear.compose.material.Scaffold
import androidx.wear.compose.material.Text
import androidx.wear.compose.material.TimeText
import androidx.wear.tooling.preview.devices.WearDevices

@Composable
fun NavigationScreen(
    distanceToNextTurn: String,
    turnIcon: ImageVector,
    remainingTime: String,
    onCancelClick: () -> Unit,
    deviceRotation: Float,
) {
    Scaffold(
        timeText = { TimeText() }
    ) {
        Column(
            modifier = Modifier.fillMaxSize(),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.SpaceAround,
        ) {
            Text(text = distanceToNextTurn)
            Icon(
                imageVector = turnIcon, contentDescription = "Turn icon", modifier = Modifier
                    .size(64.dp)
                    .rotate(deviceRotation)
            )
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Button(onClick = onCancelClick) {
                    Icon(imageVector = Icons.Default.Close, contentDescription = "Cancel")
                }
                Text(text = remainingTime)
            }
        }
    }
}

@Preview(device = WearDevices.SMALL_ROUND, showSystemUi = true)
@Composable
fun NavigationScreenPreview() {
    NavigationScreen(
        distanceToNextTurn = "100 m",
        turnIcon = Icons.Default.ArrowUpward,
        remainingTime = "5 min",
        onCancelClick = { },
        deviceRotation = 45f
    )
}
