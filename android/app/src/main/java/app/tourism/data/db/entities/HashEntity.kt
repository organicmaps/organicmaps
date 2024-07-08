package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "hashes")
data class HashEntity(
    @PrimaryKey val categoryId: Long,
    val value: String,
)
