package app.tourism.db.entities

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "feedbacks")
data class Feedback(
    @PrimaryKey(autoGenerate = true) val id: Long,
    val userId: Long,
    val message: String,
    val placeId: Long,
    val points: Int,
    val images: List<String>
)
