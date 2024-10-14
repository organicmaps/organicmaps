package app.tourism.data.remote

import app.tourism.data.dto.AllDataDto
import app.tourism.data.dto.CategoryDto
import app.tourism.data.dto.FavoritesDto
import app.tourism.data.dto.FavoritesIdsDto
import app.tourism.data.dto.auth.AuthResponseDto
import app.tourism.data.dto.auth.EmailBodyDto
import app.tourism.data.dto.place.ReviewDto
import app.tourism.data.dto.place.ReviewIdsDto
import app.tourism.data.dto.place.ReviewsDto
import app.tourism.data.dto.profile.LanguageDto
import app.tourism.data.dto.profile.ThemeDto
import app.tourism.data.dto.profile.UserData
import app.tourism.domain.models.SimpleResponse
import okhttp3.MultipartBody
import okhttp3.RequestBody
import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.Field
import retrofit2.http.FormUrlEncoded
import retrofit2.http.GET
import retrofit2.http.HTTP
import retrofit2.http.Multipart
import retrofit2.http.POST
import retrofit2.http.PUT
import retrofit2.http.Part
import retrofit2.http.Path
import retrofit2.http.Query

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

    @POST("forgot-password")
    suspend fun sendEmailForPasswordReset(@Body emailBody: EmailBodyDto): Response<SimpleResponse>
    // endregion auth

    // region profile
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

    @PUT("profile/lang")
    suspend fun updateLanguage(@Body language: LanguageDto): Response<UserData>

    @PUT("profile/theme")
    suspend fun updateTheme(@Body theme: ThemeDto): Response<UserData>
    // endregion profile

    // region places
    @GET("marks/{id}")
    suspend fun getPlacesByCategory(
        @Path("id") id: Long,
        @Query("hash") hash: String
    ): Response<CategoryDto>

    @GET("marks/all")
    suspend fun getAllPlaces(): Response<AllDataDto>
    // endregion places

    // region favorites
    @GET("favourite-marks")
    suspend fun getFavorites(): Response<FavoritesDto>

    @POST("favourite-marks")
    suspend fun addFavorites(@Body ids: FavoritesIdsDto): Response<SimpleResponse>

    @HTTP(method = "DELETE", path = "favourite-marks", hasBody = true)
    suspend fun removeFromFavorites(@Body ids: FavoritesIdsDto): Response<SimpleResponse>
    // endregion favorites

    // region reviews
    @GET("feedbacks/{id}")
    suspend fun getReviewsByPlaceId(@Path("id") id: Long): Response<ReviewsDto>

    @Multipart
    @POST("feedbacks")
    suspend fun postReview(
        @Part("message") comment: RequestBody? = null,
        @Part("mark_id") placeId: RequestBody? = null,
        @Part("points") points: RequestBody? = null,
        @Part images: List<MultipartBody.Part>? = null
    ): Response<ReviewDto>

    @HTTP(method = "DELETE", path = "feedbacks", hasBody = true)
    suspend fun deleteReviews(
        @Body feedbacks: ReviewIdsDto,
    ): Response<SimpleResponse>
    // endregion reviews
}