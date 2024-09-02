import Combine
import Foundation

protocol CurrencyService {
  func getCurrencyRates() -> AnyPublisher<CurrencyRatesDTO, ResourceError>
}

class CurrencyServiceImpl: CurrencyService {
  
  func getCurrencyRates() -> AnyPublisher<CurrencyRatesDTO, ResourceError> {
    return CombineNetworkHelper.get(path: APIEndpoints.currencyUrl)
  }
}
