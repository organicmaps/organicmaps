package app.tourism.data.repositories

import android.content.Context
import app.organicmaps.R
import app.tourism.data.db.Database
import app.tourism.data.db.entities.FavoriteSyncEntity
import app.tourism.data.db.entities.HashEntity
import app.tourism.data.db.entities.PlaceEntity
import app.tourism.data.db.entities.ReviewEntity
import app.tourism.data.dto.FavoritesIdsDto
import app.tourism.data.dto.place.PlaceDto
import app.tourism.data.remote.TourismApi
import app.tourism.data.remote.handleGenericCall
import app.tourism.data.remote.handleResponse
import app.tourism.domain.models.SimpleResponse
import app.tourism.domain.models.categories.PlaceCategory
import app.tourism.domain.models.common.PlaceShort
import app.tourism.domain.models.details.PlaceFull
import app.tourism.domain.models.details.Review
import app.tourism.domain.models.resource.Resource
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.channelFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.flow

class PlacesRepository(
    private val api: TourismApi,
    db: Database,
    @ApplicationContext private val context: Context,
) {
    private val placesDao = db.placesDao
    private val reviewsDao = db.reviewsDao
    private val hashesDao = db.hashesDao

    fun downloadAllData(): Flow<Resource<SimpleResponse>> = flow {
        val hashes = hashesDao.getHashes()
        val favoritesResponse = handleResponse(call = { api.getFavorites() }, context)

        if (hashes.isEmpty()) {
            handleGenericCall(
                call = { api.getAllPlaces() },
                mapper = { data ->
                    // get data
                    val favorites =
                        if (favoritesResponse is Resource.Success)
                            favoritesResponse.data?.data?.map {
                                it.toPlaceFull(true)
                            } else null

                    val reviews = mutableListOf<Review>()

                    fun PlaceDto.toEntity(placeCategory: PlaceCategory): PlaceEntity {
                        var placeFull = this.toPlaceFull(false)
                        placeFull =
                            placeFull.copy(
                                isFavorite = favorites?.any { it.id == placeFull.id } ?: false
                            )

                        placeFull.reviews?.let { it1 -> reviews.addAll(it1) }
                        return placeFull.toPlaceEntity(placeCategory.id)
                    }

                    val sightsEntities = data.attractions.map { placeDto ->
                        placeDto.toEntity(PlaceCategory.Sights)
                    }
                    val restaurantsEntities = data.restaurants.map { placeDto ->
                        placeDto.toEntity(PlaceCategory.Restaurants)
                    }
                    val hotelsEntities = data.accommodations.map { placeDto ->
                        placeDto.toEntity(PlaceCategory.Hotels)
                    }

                    // update places
                    placesDao.deleteAllPlaces()
                    placesDao.insertPlaces(sightsEntities)
                    placesDao.insertPlaces(restaurantsEntities)
                    placesDao.insertPlaces(hotelsEntities)

                    // update reviews
                    val reviewsEntities = reviews.map { it.toReviewEntity() }
                    reviewsDao.deleteAllReviews()
                    reviewsDao.insertReviews(reviewsEntities)

                    // update favorites
                    favorites?.forEach {
                        placesDao.setFavorite(it.id, it.isFavorite)
                    }

                    // update hashes
                    hashesDao.insertHashes(
                        listOf(
                            HashEntity(PlaceCategory.Sights.id, data.attractions_hash),
                            HashEntity(PlaceCategory.Restaurants.id, data.restaurants_hash),
                            HashEntity(PlaceCategory.Hotels.id, data.accommodations_hash),
                        )
                    )

                    // return response
                    SimpleResponse(message = context.getString(R.string.great_success))
                },
                context
            )
        } else {
            emit(Resource.Success(SimpleResponse(message = context.getString(R.string.great_success))))
        }
    }

    fun search(q: String): Flow<Resource<List<PlaceShort>>> = channelFlow {
        placesDao.search("%$q%").collectLatest { placeEntities ->
            val places = placeEntities.map { it.toPlaceShort() }
            send(Resource.Success(places))
        }
    }

    fun getPlacesByCategoryFromDbFlow(id: Long): Flow<Resource<List<PlaceShort>>> = channelFlow {
        placesDao.getPlacesByCategoryId(categoryId = id)
            .collectLatest { placeEntities ->
                send(Resource.Success(placeEntities.map { it.toPlaceShort() }))
            }
    }

    suspend fun getPlacesByCategoryFromApiIfThereIsChange(id: Long) {
        val hash = hashesDao.getHash(id)

        val favorites = placesDao.getFavoritePlaces("")
        val resource =
            handleResponse(call = { api.getPlacesByCategory(id, hash?.value ?: "") }, context)

        if (hash != null && resource is Resource.Success)
            resource.data?.let { categoryDto ->
                if (categoryDto.data.isNotEmpty()) {
                    // update places
                    placesDao.deleteAllPlacesByCategory(categoryId = id)

                    val places = categoryDto.data.map { placeDto ->
                        var placeFull = placeDto.toPlaceFull(false)
                        placeFull =
                            placeFull.copy(isFavorite = favorites.any { it.id == placeFull.id })
                        placeFull
                    }
                    placesDao.insertPlaces(places.map { it.toPlaceEntity(id) })

                    // update reviews
                    val reviewsEntities = mutableListOf<ReviewEntity>()
                    places.forEach { place ->
                        place.reviews?.map { review -> review.toReviewEntity() }
                            ?.also { reviewEntity -> reviewsEntities.addAll(reviewEntity) }
                    }
                    reviewsDao.deleteAllReviews()
                    reviewsDao.insertReviews(reviewsEntities)

                    // update hash
                    hashesDao.insertHash(hash.copy(value = categoryDto.hash))
                }
            }

    }

    fun getTopPlaces(id: Long): Flow<Resource<List<PlaceShort>>> = channelFlow {
        placesDao.getTopPlacesByCategoryId(categoryId = id)
            .collectLatest { placeEntities ->
                send(Resource.Success(placeEntities.map { it.toPlaceShort() }))
            }
    }

    fun getPlaceById(id: Long): Flow<Resource<PlaceFull>> = channelFlow {
        placesDao.getPlaceById(id).collectLatest { placeEntity ->
            if(placeEntity != null)
                send(Resource.Success(placeEntity.toPlaceFull()))
            else
                send(Resource.Error(message = "Не найдено"))
        }
    }

    fun getFavorites(q: String): Flow<Resource<List<PlaceShort>>> = channelFlow {
        placesDao.getFavoritePlacesFlow("%$q%")
            .collectLatest { placeEntities ->
                send(Resource.Success(placeEntities.map { it.toPlaceShort() }))
            }
    }

    suspend fun setFavorite(placeId: Long, isFavorite: Boolean) {
        placesDao.setFavorite(placeId, isFavorite)

        val favoritesIdsDto = FavoritesIdsDto(marks = listOf(placeId))

        val favoriteSyncEntity = FavoriteSyncEntity(placeId, isFavorite)
        placesDao.addFavoriteSync(favoriteSyncEntity)
        val response: Resource<SimpleResponse> = if (isFavorite)
            handleResponse(call = { api.addFavorites(favoritesIdsDto) }, context)
        else
            handleResponse(call = { api.removeFromFavorites(favoritesIdsDto) }, context)

        if (response is Resource.Success)
            placesDao.removeFavoriteSync(favoriteSyncEntity.placeId)
        else if (response is Resource.Error)
            placesDao.addFavoriteSync(favoriteSyncEntity)
    }

    suspend fun syncFavorites() {
        val favoritesToSyncEntities = placesDao.getFavoriteSyncData()

        val favoritesToAdd = favoritesToSyncEntities.filter { it.isFavorite }.map { it.placeId }
        val favoritesToRemove = favoritesToSyncEntities.filter { !it.isFavorite }.map { it.placeId }

        if (favoritesToAdd.isNotEmpty()) {
            val responseToAddFavs =
                handleResponse(
                    call = { api.addFavorites(FavoritesIdsDto(favoritesToAdd)) },
                    context
                )
            if (responseToAddFavs is Resource.Success) {
                placesDao.removeFavoriteSync(favoritesToAdd)
            }
        }

        if (favoritesToRemove.isNotEmpty()) {
            val responseToRemoveFavs =
                handleResponse(
                    call = { api.removeFromFavorites(FavoritesIdsDto(favoritesToRemove)) },
                    context
                )
            if (responseToRemoveFavs is Resource.Success) {
                placesDao.removeFavoriteSync(favoritesToRemove)
            }
        }
    }
}
