package app.tourism.data.repositories

import android.content.Context
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleCall
import app.tourism.data.remote.toFormDataRequestBody
import app.tourism.domain.models.profile.PersonalData
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.asRequestBody
import java.io.File

class ProfileRepository(
    private val api: TourismApi,
    private val userPreferences: UserPreferences,
    @ApplicationContext private val context: Context
) {
    fun getPersonalData(): Flow<Resource<PersonalData>> = flow {
        handleCall(
            call = { api.getUser() },
            mapper = {
                it.data.toPersonalData()
            }
        )
    }

    fun updateProfile(
        fullName: String,
        country: String,
        email: String?,
        pfpFile: File?
    ): Flow<Resource<PersonalData>> =
        flow {
            var pfpMultipart: MultipartBody.Part? = null
            if (pfpFile != null) {
                val requestBody = pfpFile.asRequestBody("image/*".toMediaType())
                pfpMultipart =
                    MultipartBody.Part.createFormData("avatar", pfpFile.name, requestBody)
            }

            val language = userPreferences.getLanguage()?.code
            val theme = userPreferences.getTheme()?.code

            handleCall(
                call = {
                    api.updateProfile(
                        fullName = fullName.toFormDataRequestBody(),
                        email = email?.toFormDataRequestBody(),
                        country = country.toFormDataRequestBody(),
                        language = language.toString().toFormDataRequestBody(),
                        theme = theme.toString().toFormDataRequestBody(),
                        avatar = pfpMultipart
                    )
                },
                mapper = { it.data.toPersonalData() }
            )
        }

    suspend fun updateLanguage(code: String) {
        try {
            api.updateProfile(language = code.toFormDataRequestBody())
        } catch (e: Exception) {
            println(e.message)
        }
    }

    suspend fun updateTheme(code: String) {
        try {
            api.updateProfile(theme = code.toFormDataRequestBody())
        } catch (e: Exception) {
            println(e.message)
        }
    }
}