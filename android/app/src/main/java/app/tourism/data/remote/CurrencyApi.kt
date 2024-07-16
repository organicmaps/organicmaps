package app.tourism.data.remote

import app.tourism.data.dto.CurrencyRatesDataDto
import retrofit2.Response
import retrofit2.http.GET

interface CurrencyApi {

    @GET("currency")
    suspend fun getCurrency(): Response<CurrencyRatesDataDto>
}
