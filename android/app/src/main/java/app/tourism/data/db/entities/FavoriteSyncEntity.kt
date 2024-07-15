package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "favorites_to_sync")
data class FavoriteSyncEntity(
    @PrimaryKey val placeId: Long,
    val isFavorite: Boolean,
)