package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.FavoriteSyncEntity
import app.tourism.data.db.entities.PlaceEntity
import kotlinx.coroutines.flow.Flow

@Dao
interface PlacesDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertPlaces(places: List<PlaceEntity>)

    @Query("DELETE FROM places")
    suspend fun deleteAllPlaces()

    @Query("DELETE FROM places WHERE categoryId = :categoryId")
    suspend fun deleteAllPlacesByCategory(categoryId: Long)

    @Query("SELECT * FROM places WHERE UPPER(name) LIKE UPPER(:q)")
    fun search(q: String = ""): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE categoryId = :categoryId")
    fun getPlacesByCategoryId(categoryId: Long): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE categoryId =:categoryId ORDER BY rating DESC LIMIT 15")
    fun getTopPlacesByCategoryId(categoryId: Long): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE id = :placeId")
    fun getPlaceById(placeId: Long): Flow<PlaceEntity?>

    @Query("SELECT * FROM places WHERE isFavorite = 1 AND UPPER(name) LIKE UPPER(:q)")
    fun getFavoritePlacesFlow(q: String = ""): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE isFavorite = 1 AND UPPER(name) LIKE UPPER(:q)")
    fun getFavoritePlaces(q: String = ""): List<PlaceEntity>

    @Query("UPDATE places SET isFavorite = :isFavorite WHERE id = :placeId")
    suspend fun setFavorite(placeId: Long, isFavorite: Boolean)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun addFavoriteSync(favoriteSyncEntity: FavoriteSyncEntity)

    @Query("DELETE FROM favorites_to_sync WHERE placeId = :placeId")
    suspend fun removeFavoriteSync(placeId: Long)

    @Query("DELETE FROM favorites_to_sync WHERE placeId in (:placeId)")
    suspend fun removeFavoriteSync(placeId: List<Long>)

    @Query("SELECT * FROM favorites_to_sync")
    fun getFavoriteSyncData(): List<FavoriteSyncEntity>
}
