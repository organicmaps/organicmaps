package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.CurrencyRatesEntity

@Dao
interface CurrencyRatesDao {

    @Query("SELECT * FROM currency_rates")
    suspend fun getCurrencyRates(): CurrencyRatesEntity?

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun updateCurrencyRates(entity: CurrencyRatesEntity)
}