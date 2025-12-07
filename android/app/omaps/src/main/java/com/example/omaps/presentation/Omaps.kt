/* While this template provides a good starting point for using Wear Compose, you can always
 * take a look at https://github.com/android/wear-os-samples/tree/main/ComposeStarter to find the
 * most up to date changes to the libraries and their usages.
 */

package com.example.omaps.presentation

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.example.omaps.presentation.navigation.SensorScreen
import com.example.omaps.presentation.search.SearchScreen
import com.example.omaps.presentation.theme.OrganicMapsTheme

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
    var isNavigating by remember { mutableStateOf(false) }
    OrganicMapsTheme {
        if (isNavigating) {
            SensorScreen(
                onCancelClick = { isNavigating = false }
            )
        } else {
            SearchScreen(onSearchClick = { isNavigating = true })
        }
    }
}
