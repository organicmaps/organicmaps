package app.tourism.di

import android.content.Context
import app.tourism.BASE_URL
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.remote.CurrencyApi
import app.tourism.data.remote.TourismApi
import app.tourism.data.repositories.CurrencyRepository
import app.tourism.db.Database
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import retrofit2.converter.simplexml.SimpleXmlConverterFactory
import java.util.concurrent.TimeUnit
import javax.inject.Named
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {
    @Provides
    @Singleton
    fun provideApi(@Named(MAIN_OKHTTP_LABEL) okHttpClient: OkHttpClient): TourismApi {
        return Retrofit.Builder()
            .baseUrl(BASE_URL)
            .addConverterFactory(GsonConverterFactory.create())
            .client(okHttpClient)
            .build()
            .create(TourismApi::class.java)
    }

    @Provides
    @Singleton
    @Named(MAIN_OKHTTP_LABEL)
    fun provideHttpClient(@ApplicationContext context: Context, userPreferences: UserPreferences): OkHttpClient {
        return OkHttpClient.Builder()
            .addInterceptor(
                HttpLoggingInterceptor()
                    .setLevel(HttpLoggingInterceptor.Level.BODY)
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
    @Named(CURRENCY_OKHTTP_LABEL)
    fun provideHttpClientForCurrencyRetrofit(): OkHttpClient {
        val okHttpClient = OkHttpClient.Builder()
        okHttpClient.readTimeout(1, TimeUnit.MINUTES)
        okHttpClient.connectTimeout(1, TimeUnit.MINUTES)
            .addInterceptor(
                HttpLoggingInterceptor()
                    .setLevel(HttpLoggingInterceptor.Level.BODY)
            )

        return okHttpClient.build()
    }

    @Provides
    @Singleton
    @Named(CURRENCY_RETROFIT_LABEL)
    fun provideCurrencyRetrofit(@Named(CURRENCY_OKHTTP_LABEL) client: OkHttpClient): Retrofit {
        return Retrofit.Builder()
            .baseUrl(CurrencyApi.BASE_URL)
            .addConverterFactory(SimpleXmlConverterFactory.create())
            .client(client)
            .build()
    }

    @Provides
    @Singleton
    fun provideCurrencyApi(@Named(CURRENCY_RETROFIT_LABEL) retrofit: Retrofit): CurrencyApi {
        return retrofit.create(CurrencyApi::class.java)
    }
}

const val MAIN_OKHTTP_LABEL = "main okhttp"
const val CURRENCY_RETROFIT_LABEL = "currency retrofit"
const val CURRENCY_OKHTTP_LABEL = "currency okhttp"