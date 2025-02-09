package app.tourism.data.db

import androidx.room.Database
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import app.tourism.data.db.dao.CurrencyRatesDao
import app.tourism.data.db.dao.HashesDao
import app.tourism.data.db.dao.PlacesDao
import app.tourism.data.db.dao.ReviewsDao
import app.tourism.data.db.entities.CurrencyRatesEntity
import app.tourism.data.db.entities.FavoriteSyncEntity
import app.tourism.data.db.entities.HashEntity
import app.tourism.data.db.entities.PlaceEntity
import app.tourism.data.db.entities.ReviewEntity
import app.tourism.data.db.entities.ReviewPlannedToPostEntity

@Database(
    entities = [
        PlaceEntity::class,
        ReviewEntity::class,
        ReviewPlannedToPostEntity::class,
        FavoriteSyncEntity::class,
        HashEntity::class,
        CurrencyRatesEntity::class
    ],
    version = 2,
    exportSchema = false
)
@TypeConverters(Converters::class)
abstract class Database : RoomDatabase() {
    abstract val currencyRatesDao: CurrencyRatesDao
    abstract val placesDao: PlacesDao
    abstract val hashesDao: HashesDao
    abstract val reviewsDao: ReviewsDao
}
