package app.tourism.db

import androidx.room.Database
import androidx.room.RoomDatabase
import app.tourism.db.dao.CurrencyRatesDao
import app.tourism.db.entities.CurrencyRatesEntity

@Database(
    entities = [CurrencyRatesEntity::class],
    version = 1,
    exportSchema = false
)

abstract class Database: RoomDatabase() {
    abstract val currencyRatesDao: CurrencyRatesDao
}