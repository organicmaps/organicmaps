package app.tourism.data.db.entities

import androidx.room.Entity
import androidx.room.PrimaryKey
import app.tourism.domain.models.profile.CurrencyRates

@Entity(tableName = "currency_rates")
data class CurrencyRatesEntity(
    @PrimaryKey
    val id: Long,
    val usd: Double,
    val eur: Double,
    val rub: Double,
) {
    fun toCurrencyRates() = CurrencyRates(usd, eur, rub)
}