extension CurrencyRatesEntity {
  func toCurrencyRates() -> CurrencyRates {
    return CurrencyRates(usd: usd, eur: eur, rub: rub)
  }
}
