package app.tourism.data.remote

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.net.NetworkInfo
import android.net.wifi.WifiManager
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.PlacesRepository
import app.tourism.data.repositories.ProfileRepository
import app.tourism.data.repositories.ReviewsRepository
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import javax.inject.Inject

@AndroidEntryPoint
class WifiReceiver : BroadcastReceiver() {
    @Inject
    lateinit var reviewsRepository: ReviewsRepository

    @Inject
    lateinit var placesRepository: PlacesRepository

    @Inject
    lateinit var profileRepository: ProfileRepository

    @Inject
    lateinit var userPreferences: UserPreferences

    override fun onReceive(context: Context, intent: Intent) {
        val info: NetworkInfo? = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO)

        if (info != null && info.isConnected) {
            CoroutineScope(Dispatchers.IO).launch {
                delay(2000L) // to avoid errors
                CoroutineScope(Dispatchers.IO).launch { reviewsRepository.syncReviews() }
                CoroutineScope(Dispatchers.IO).launch { placesRepository.syncFavorites() }
                CoroutineScope(Dispatchers.IO).launch { profileRepository.syncLanguageIfNecessary() }
            }
        }
    }
}

