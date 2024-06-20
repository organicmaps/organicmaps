package app.tourism.data.repositories

import app.tourism.Constants
import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleCall
import app.tourism.domain.models.profile.PersonalData
import app.tourism.domain.models.resource.Resource
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow

class ProfileRepository(private val api: TourismApi) {
    fun getPersonalData(): Flow<Resource<PersonalData>> = flow {
        handleCall(
            call = { api.getUser() },
            mapper = {
                // todo api request not finished yet
                PersonalData(
                    fullName = "Emin A.",
                    country = "TJ",
                    pfpUrl = Constants.IMAGE_URL_EXAMPLE,
                    phone = "+992 987654321",
                    email = "ohhhcmooonmaaaaaaaaaaan@gmail.com",
                )
            }
        )
    }
}