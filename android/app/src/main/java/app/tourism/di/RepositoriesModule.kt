package app.tourism.di

import android.content.Context
import app.tourism.data.db.Database
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.remote.CurrencyApi
import app.tourism.data.remote.TourismApi
import app.tourism.data.repositories.AuthRepository
import app.tourism.data.repositories.CurrencyRepository
import app.tourism.data.repositories.PlacesRepository
import app.tourism.data.repositories.ProfileRepository
import app.tourism.data.repositories.ReviewsRepository
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object RepositoriesModule {
    @Provides
    @Singleton
    fun provideAuthRepository(
        api: TourismApi,
        @ApplicationContext context: Context,
    ): AuthRepository {
        return AuthRepository(api, context)
    }

    @Provides
    @Singleton
    fun providePlacesRepository(
        api: TourismApi,
        db: Database,
        userPreferences: UserPreferences,
        @ApplicationContext context: Context,
    ): PlacesRepository {
        return PlacesRepository(api, db, userPreferences, context)
    }

    @Provides
    @Singleton
    fun provideReviewsRepository(
        api: TourismApi,
        db: Database,
        @ApplicationContext context: Context,
    ): ReviewsRepository {
        return ReviewsRepository(context, api, db)
    }


    @Provides
    @Singleton
    fun provideProfileRepository(
        api: TourismApi,
        userPreferences: UserPreferences,
        @ApplicationContext context: Context,
    ): ProfileRepository {
        return ProfileRepository(api, userPreferences, context)
    }

    @Provides
    @Singleton
    fun provideCurrencyRepository(
        api: CurrencyApi,
        db: Database,
        @ApplicationContext context: Context,
    ): CurrencyRepository {
        return CurrencyRepository(api, db, context )
    }
}