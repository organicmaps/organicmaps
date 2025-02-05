package app.tourism.di

import android.content.Context
import app.tourism.BASE_URL
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.remote.CurrencyApi
import app.tourism.data.remote.TourismApi
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import javax.inject.Named
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {
    @Provides
    @Singleton
    fun provideApi(okHttpClient: OkHttpClient): TourismApi {
        return Retrofit.Builder()
            .baseUrl("$BASE_URL/api/")
            .addConverterFactory(GsonConverterFactory.create())
            .client(okHttpClient)
            .build()
            .create(TourismApi::class.java)
    }

    @Provides
    @Singleton
    fun provideHttpClient(
        @ApplicationContext context: Context,
        userPreferences: UserPreferences
    ): OkHttpClient {
        return OkHttpClient.Builder()
            .addInterceptor(
                HttpLoggingInterceptor()
                    .setLevel(HttpLoggingInterceptor.Level.BASIC)
            )
            .addInterceptor { chain ->
                val original = chain.request()
                original.body.toString()

                val requestBuilder = original.newBuilder()

                userPreferences.getToken()?.let {
                    requestBuilder.addHeader("Authorization", "Bearer $it")
                }

                val request = requestBuilder
                    .addHeader("Accept", "application/json")
                    .method(original.method, original.body).build()

                chain.proceed(request)

            }.build()
    }

    @Provides
    @Singleton
    @Named(CURRENCY_RETROFIT_LABEL)
    fun provideCurrencyRetrofit(client: OkHttpClient): Retrofit {
        return Retrofit.Builder()
            .baseUrl("$BASE_URL/api/")
            .addConverterFactory(GsonConverterFactory.create())
            .client(client)
            .build()
    }

    @Provides
    @Singleton
    fun provideCurrencyApi(@Named(CURRENCY_RETROFIT_LABEL) retrofit: Retrofit): CurrencyApi {
        return retrofit.create(CurrencyApi::class.java)
    }
}

const val CURRENCY_RETROFIT_LABEL = "currency retrofit"
