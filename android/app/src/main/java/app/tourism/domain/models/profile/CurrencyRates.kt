package app.tourism.domain.models.profile

import app.tourism.db.entities.CurrencyRatesEntity

data class CurrencyRates(val usd: Double, val eur: Double, val rub: Double) {
    fun toCurrencyRatesEntity() = CurrencyRatesEntity(1, usd, eur, rub)
}
