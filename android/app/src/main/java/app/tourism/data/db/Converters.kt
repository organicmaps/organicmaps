package app.tourism.data.db

import androidx.room.TypeConverter
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json

class Converters {
   @TypeConverter
   fun fromList(value : List<String>) = Json.encodeToString(value)

   @TypeConverter
   fun toList(value: String) = Json.decodeFromString<List<String>>(value)
}