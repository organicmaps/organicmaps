package app.tourism.data.repositories

import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleGenericCall
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.auth.AuthResponse
import app.tourism.domain.models.auth.RegistrationData
import app.tourism.domain.models.resource.Resource
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow

class AuthRepository(private val api: TourismApi) {
    fun signIn(email: String, password: String): Flow<Resource<AuthResponse>> = flow {
        handleGenericCall(
            call = { api.signIn(email, password) },
            mapper = { it.toAuthResponse() }
        )
    }

    fun signUp(registrationData: RegistrationData) = flow {
        handleGenericCall(
            call = {
                api.signUp(
                    registrationData.fullName,
                    registrationData.email,
                    registrationData.password,
                    registrationData.passwordConfirmation,
                    registrationData.country
                )
            },
            mapper = { it.toAuthResponse() }
        )
    }

    fun signOut(): Flow<Resource<SimpleResponse>> = flow {
        handleGenericCall(
            call = { api.signOut() },
            mapper = { it }
        )
    }
}