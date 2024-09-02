extension CurrencyRatesEntity {
  func toCurrencyRates() -> CurrencyRates {
    return CurrencyRates(usd: usd, eur: eur, rub: rub)
  }
}

extension PersonalDataEntity {
  func toPersonalData() -> PersonalData {
    return PersonalData(
      id: self.id,
      fullName: self.fullName ?? "",
      country: self.country ?? "",
      pfpUrl: self.pfpUrl,
      email: self.email ?? "",
      language: self.language,
      theme: self.theme
    )
  }
}
