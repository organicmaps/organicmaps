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

    @Query("DELETE FROM places WHERE id IN (:idsList)")
    suspend fun deletePlaces(idsList: List<Long>)

    @Query("DELETE FROM places")
    suspend fun deleteAllPlaces()

    @Query("DELETE FROM places WHERE categoryId = :categoryId AND language =:language")
    suspend fun deleteAllPlacesByCategory(categoryId: Long, language: String)

    @Query("SELECT * FROM places WHERE UPPER(name) LIKE UPPER(:q) AND language =:language")
    fun search(q: String = "", language: String): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE categoryId = :categoryId AND language =:language ORDER BY rating DESC, name ASC")
    fun getSortedPlacesByCategoryIdFlow(categoryId: Long, language: String): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE categoryId = :categoryId AND language =:language")
    fun getPlacesByCategoryIdNotFlow(categoryId: Long, language: String): List<PlaceEntity>

    @Query("SELECT * FROM places WHERE categoryId =:categoryId AND language =:language ORDER BY rating DESC LIMIT 15")
    fun getTopPlacesByCategoryId(categoryId: Long, language: String): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE id = :placeId AND language =:language")
    fun getPlaceById(placeId: Long, language: String): Flow<PlaceEntity?>

    @Query("SELECT * FROM places WHERE isFavorite = 1 AND UPPER(name) LIKE UPPER(:q) AND language =:language")
    fun getFavoritePlacesFlow(q: String = "", language: String): Flow<List<PlaceEntity>>

    @Query("SELECT * FROM places WHERE isFavorite = 1 AND UPPER(name) LIKE UPPER(:q) AND language =:language")
    fun getFavoritePlaces(q: String = "", language: String): List<PlaceEntity>

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
