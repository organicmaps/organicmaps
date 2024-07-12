package app.tourism.data.remote

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.net.ConnectivityManager
import android.net.ConnectivityManager.NetworkCallback
import android.net.Network
import android.net.NetworkRequest
import app.tourism.data.repositories.PlacesRepository
import app.tourism.data.repositories.ReviewsRepository
import javax.inject.Inject

class WifiReceiver : BroadcastReceiver() {
    @Inject
    lateinit var reviewsRepository: ReviewsRepository

    @Inject
    lateinit var placesRepository: PlacesRepository

    override fun onReceive(context: Context, intent: Intent) {
        val cm = context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val builder = NetworkRequest.Builder()

        cm.registerNetworkCallback(
            builder.build(),
            object : NetworkCallback() {
                override fun onAvailable(network: Network) {
                    super.onAvailable(network)

                    reviewsRepository.syncReviews()
                    placesRepository.syncFavorites()
                }
            }
        )
    }
}