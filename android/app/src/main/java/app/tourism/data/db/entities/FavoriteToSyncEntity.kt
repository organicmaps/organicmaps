package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "favorites_to_sync")
data class FavoriteToSyncEntity(
    @PrimaryKey val placeId: Long,
    val isFavorite: Boolean,
)