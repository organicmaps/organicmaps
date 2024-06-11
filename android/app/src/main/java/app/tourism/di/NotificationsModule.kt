package app.tourism.di

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationCompat.VISIBILITY_PRIVATE
import androidx.core.app.NotificationManagerCompat
import app.organicmaps.R
import app.tourism.DownloaderService
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Qualifier

@Module
@InstallIn(SingletonComponent::class)
object NotificationModule {

    @Provides
    @NotInProgressNotificationCompatBuilder
    fun provideNotInProgressNotificationBuilder(
        @ApplicationContext context: Context
    ): NotificationCompat.Builder {
        return NotificationCompat.Builder(context, DownloaderService.NOTIFICATION_CHANNEL)
            .setContentTitle("Welcome")
            .setSmallIcon(R.mipmap.ic_launcher_round)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setVisibility(VISIBILITY_PRIVATE)
    }

    @Provides
    @InProgressNotificationCompatBuilder
    fun provideInProgressNotificationBuilder(
        @ApplicationContext context: Context
    ): NotificationCompat.Builder {
        return NotificationCompat.Builder(context, DownloaderService.DOWNLOADING_CHANNEL)
                .setOngoing(true)
                .setContentTitle(context.getString(R.string.map_downloading))
                .setSmallIcon(R.mipmap.ic_launcher_round)
                .setSilent(true)
                .setPriority(NotificationCompat.PRIORITY_LOW)
    }

    @Provides
    fun provideNotificationManager(
        @ApplicationContext context: Context
    ): NotificationManagerCompat {
        val notificationManager = NotificationManagerCompat.from(context)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                DownloaderService.NOTIFICATION_CHANNEL,
                DownloaderService.NOTIFICATION_CHANNEL,
                NotificationManager.IMPORTANCE_DEFAULT
            )
            val channel2 = NotificationChannel(
                DownloaderService.DOWNLOADING_CHANNEL,
                DownloaderService.DOWNLOADING_CHANNEL,
                NotificationManager.IMPORTANCE_LOW
            )
            notificationManager.createNotificationChannel(channel)
            notificationManager.createNotificationChannel(channel2)
        }
        return notificationManager
    }

    @Provides
    fun provideString(): String = "sdklfjsdalkfj"
}

@Qualifier
@Retention(AnnotationRetention.BINARY)
annotation class NotInProgressNotificationCompatBuilder

@Qualifier
@Retention(AnnotationRetention.BINARY)
annotation class InProgressNotificationCompatBuilder