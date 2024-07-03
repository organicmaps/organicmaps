package app.tourism.data.remote

import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.resource.Resource
import com.google.gson.Gson
import kotlinx.coroutines.flow.FlowCollector
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONException
import retrofit2.HttpException
import retrofit2.Response
import java.io.IOException

suspend inline fun <T, reified R> FlowCollector<Resource<R>>.handleCall(
    call: () -> Response<T>,
    mapper: (T) -> R,
    emitLoadingStatusBeforeCall: Boolean = true
) {
    if(emitLoadingStatusBeforeCall) emit(Resource.Loading())
    try {
        val response = call()
        val body = response.body()?.let { mapper(it) }
        if (response.isSuccessful) emit(Resource.Success(body))

        else emit(response.parseError())
    } catch(e: HttpException) {
        emit(
            Resource.Error(
            message = "Упс! Что-то пошло не так."
        ))
    } catch(e: IOException) {
        emit(
            Resource.Error(
            message = "Не удается соединиться с сервером, проверьте интернет подключение"
        ))
    }
}

inline fun <T, reified R> Response<T>.parseError(): Resource<R> {
    return try {
        val response = Gson()
            .fromJson(
                errorBody()?.string().toString(),
                SimpleResponse::class.java
            )

        Resource.Error(message = response?.message ?: "")
    } catch (e: JSONException) {
        println(e.message)
        Resource.Error(e.toString())
    }
}

fun String.toFormDataRequestBody() = this.toRequestBody("multipart/form-data".toMediaTypeOrNull())

