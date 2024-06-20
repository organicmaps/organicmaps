package app.tourism.di

import app.tourism.data.remote.TourismApi
import app.tourism.data.repositories.AuthRepository
import app.tourism.data.repositories.ProfileRepository
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object RepositoriesModule {
    @Provides
    @Singleton
    fun provideAuthRepository(api: TourismApi): AuthRepository {
        return AuthRepository(api)
    }

    @Provides
    @Singleton
    fun provideProfileRepository(api: TourismApi): ProfileRepository {
        return ProfileRepository(api)
    }
}