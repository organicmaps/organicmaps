package app.tourism.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.db.entities.CurrencyRatesEntity

@Dao
interface CurrencyRatesDao {

    @Query("SELECT * FROM currency_rates")
    fun getCurrencyRates(): CurrencyRatesEntity?

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun updateCurrencyRates(entity: CurrencyRatesEntity)
}