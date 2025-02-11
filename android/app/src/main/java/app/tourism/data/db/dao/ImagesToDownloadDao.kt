package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.ImageToDownloadEntity

@Dao
interface ImagesToDownloadDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertImages(places: List<ImageToDownloadEntity>)

    @Query("UPDATE images_to_download SET downloaded = :downloaded WHERE url = :url")
    suspend fun markAsDownloaded(url: String, downloaded: Boolean)

    @Query("UPDATE images_to_download SET downloaded = 0")
    suspend fun markAllImagesAsNotDownloaded()

    @Query("SELECT * FROM images_to_download")
    suspend fun getAllImages(): List<ImageToDownloadEntity>
}
