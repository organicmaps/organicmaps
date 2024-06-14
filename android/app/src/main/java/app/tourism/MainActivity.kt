package app.tourism

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat.startActivity
import app.organicmaps.DownloadResourcesLegacyActivity
import app.organicmaps.downloader.CountryItem
import app.tourism.data.dto.SiteLocation
import app.tourism.ui.theme.OrganicMapsTheme
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        navigateToMapToDownloadIfNotPresent()
//        navigateToAuthIfNotAuthed()

        enableEdgeToEdge()
        setContent {
            OrganicMapsTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        name = "Android",
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }

    private fun navigateToMapToDownloadIfNotPresent() {
        val mCurrentCountry = CountryItem.fill("Tajikistan")
        if(!mCurrentCountry.present) {
            val intent = Intent(this, DownloadResourcesLegacyActivity::class.java)
            startActivity(this, intent, null)
        }
    }

    private fun navigateToAuthIfNotAuthed() {
        val intent = Intent(this, AuthActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK or Intent.FLAG_ACTIVITY_NEW_TASK)
        startActivity(this, intent, null)
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    val context = LocalContext.current
    Column {
        Text(
            text = "Hello $name!",
            modifier = modifier
        )
        Button(
            onClick = {
                val intent = Intent(context, DownloadResourcesLegacyActivity::class.java)
                intent.putExtra(
                    "end_point",
                    SiteLocation("Name", 38.573, 68.807)
                )
                startActivity(context, intent, null)
            },
        ) {
            Text(text = "navigate to Map", modifier = modifier)
        }
    }
}
