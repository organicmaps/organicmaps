package app.tourism.data.remote

import app.tourism.data.dto.auth.AuthResponseDto
import app.tourism.data.dto.profile.User
import app.tourism.domain.models.SimpleResponse
import retrofit2.Response
import retrofit2.http.Field
import retrofit2.http.FormUrlEncoded
import retrofit2.http.GET
import retrofit2.http.POST

interface TourismApi {
    // region auth
    @FormUrlEncoded
    @POST("login")
    suspend fun signIn(
        @Field("username") username: String,
        @Field("password") password: String,
    ): Response<AuthResponseDto>

    @FormUrlEncoded
    @POST("register")
    suspend fun signUp(
        @Field("full_name") fullName: String,
        @Field("username") username: String,
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
    suspend fun getUser(): Response<User>
    // endregion profile

}