package app.tourism.data.remote

import app.tourism.data.dto.currency.CurrenciesList
import app.tourism.domain.models.resource.Resource
import app.tourism.utils.getCurrentDate
import app.tourism.utils.getCurrentLocale
import com.google.gson.JsonParseException
import retrofit2.HttpException
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Query
import java.io.IOException

interface CurrencyApi {

    @GET("en/kurs/export_xml.php")
    suspend fun getCurrency(
        @Query("date") date: String = getCurrentDate(),
        @Query("export") export: String = "xmlout"
    ): Response<CurrenciesList>

    companion object {
        const val BASE_URL = "http://nbt.tj/"
    }
}
