package app.tourism.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.Relation

@Entity(tableName = "Places")
data class Place(
    @PrimaryKey(autoGenerate = true) val id: Long,
    val name: String,
    val phone: String,
    val shortDescription: String,
    val longDescription: String,
    val cover: String,
    val gallery: List<String>,
    @Relation(parentColumn = "id", entityColumn = "placeId", entity = Feedback::class)
    val feedbacks: List<Feedback>,
    val coordinates: Coordinates,
    val rating: Double,
    val isFavorite: Boolean
)

data class Coordinates(
    val latitude: String,
    val longitude: String
)