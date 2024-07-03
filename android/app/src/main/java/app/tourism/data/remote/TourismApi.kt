package app.tourism.data.remote

import app.tourism.data.dto.auth.AuthResponseDto
import app.tourism.data.dto.profile.UserData
import app.tourism.domain.models.SimpleResponse
import okhttp3.MultipartBody
import okhttp3.RequestBody
import retrofit2.Response
import retrofit2.http.Field
import retrofit2.http.FormUrlEncoded
import retrofit2.http.GET
import retrofit2.http.Multipart
import retrofit2.http.POST
import retrofit2.http.Part

interface TourismApi {
    // region auth
    @FormUrlEncoded
    @POST("login")
    suspend fun signIn(
        @Field("email") email: String,
        @Field("password") password: String,
    ): Response<AuthResponseDto>

    @FormUrlEncoded
    @POST("register")
    suspend fun signUp(
        @Field("full_name") fullName: String,
        @Field("email") email: String,
        @Field("password") password: String,
        @Field("password_confirmation") passwordConfirmation: String,
        @Field("country") country: String,
    ): Response<AuthResponseDto>

    @POST("logout")
    suspend fun signOut(): Response<SimpleResponse>
    // endregion auth

    // region profile
    // todo api request not finished yet
    @GET("user")
    suspend fun getUser(): Response<UserData>

    @Multipart
    @POST("profile")
    suspend fun updateProfile(
        @Part("full_name") fullName: RequestBody? = null,
        @Part("email") email: RequestBody? = null,
        @Part("country") country: RequestBody? = null,
        @Part("language") language: RequestBody? = null,
        @Part("theme") theme: RequestBody? = null,
        @Part("_method") _method: RequestBody? = "PUT".toFormDataRequestBody(),
        @Part avatar: MultipartBody.Part? = null
    ): Response<UserData>
    // endregion profile

}