import Foundation

enum CurrencyConversionError: Error {
  case invalidData
}

struct CurrencyRatesDTO: Codable {
  let data: CurrencyDataDTO
  
  struct CurrencyDataDTO: Codable {
    let usd: String
    let eur: String
    let rub: String
  }
}

extension CurrencyRatesDTO {
  func toCurrencyRates() throws -> CurrencyRates {
    guard let usd = Double(data.usd),
          let eur = Double(data.eur),
          let rub = Double(data.rub) else {
      throw CurrencyConversionError.invalidData
    }
    return CurrencyRates(usd: usd, eur: eur, rub: rub)
  }
}
