package app.tourism

import android.Manifest
import android.content.Intent
import android.os.Build
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
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat.startActivity
import app.organicmaps.DownloadResourcesLegacyActivity
import app.organicmaps.downloader.CountryItem
import app.tourism.data.dto.SiteLocation
import app.tourism.ui.theme.OrganicMapsTheme
import dagger.hilt.android.AndroidEntryPoint
import javax.inject.Inject

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(Manifest.permission.POST_NOTIFICATIONS),
                0
            )
        }

        enableEdgeToEdge()
        setContent {
            OrganicMapsTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Column(Modifier.padding(innerPadding)) {
                        TestingStuffs()
                    }
                }
            }
        }
    }

    @Composable
    fun TestingStuffs() {
        Column {
            TestingNavigationToMap()
            TestingNotifs()
        }
    }

    @Composable
    fun TestingNavigationToMap() {
        Button(
            onClick = {
                val intent = Intent(this, DownloadResourcesLegacyActivity::class.java)
                intent.putExtra(
                    "end_point",
                    SiteLocation("Name", 38.573, 68.807)
                )
                startActivity(this, intent, null)
            },
        ) {
            Text(text = "navigate to Map")
        }
    }

    @Composable
    fun TestingNotifs() {
        Button(
            onClick = {
                Intent(applicationContext, DownloaderService::class.java).also {
                    val mCurrentCountry = CountryItem.fill("Tajikistan")
                    if (!mCurrentCountry.present) {
                        it.action = DownloaderService.Actions.START.toString()
                        startService(it)
                    }
                }
            },
        ) {
            Text(text = "Start download")
        }
        Button(
            onClick = {
                Intent(applicationContext, DownloaderService::class.java).also {
                    it.action = DownloaderService.Actions.STOP.toString()
                    startService(it)
                }
            },
        ) {
            Text(text = "Stop download")
        }
    }
}
