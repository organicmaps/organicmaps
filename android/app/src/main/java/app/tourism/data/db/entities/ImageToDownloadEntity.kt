package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "images_to_download")
data class ImageToDownloadEntity(
    @PrimaryKey
    val url: String,
    val downloaded: Boolean
)