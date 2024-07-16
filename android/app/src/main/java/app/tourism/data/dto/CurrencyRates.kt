package app.tourism.data.dto

import app.tourism.data.db.entities.CurrencyRatesEntity
import app.tourism.domain.models.profile.CurrencyRates

data class CurrencyRatesDataDto(val data: CurrencyRatesDto) {
    fun toCurrencyRates() = CurrencyRates(data.usd, data.eur, data.rub)
    fun toCurrencyRatesEntity() = CurrencyRatesEntity(1, data.usd, data.eur, data.rub)
}

data class CurrencyRatesDto(val usd: Double, val eur: Double, val rub: Double)