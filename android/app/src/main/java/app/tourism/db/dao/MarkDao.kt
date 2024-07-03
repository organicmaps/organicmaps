package app.tourism.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import app.tourism.db.entities.Place
import kotlinx.coroutines.flow.Flow

@Dao
interface PlaceDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertPlaces(places: List<Place>)

    @Query("DELETE FROM Places")
    suspend fun deleteAllPlaces()

    @Query("SELECT * FROM Places")
    suspend fun getAllPlaces(): Flow<List<Place>>

    @Query("SELECT * FROM Places WHERE id = :placeId")
    suspend fun getPlaceById(placeId: Long): Flow<Place>

    @Query("SELECT * FROM Places WHERE isFavorite == 1")
    suspend fun getFavoritePlaces(): Flow<List<Place>>

    @Update
    suspend fun setFavorite(placeId: Long, isFavorite: Boolean)
}
