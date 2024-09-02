import Combine

protocol CurrencyRepository {
  var currencyPassThroughSubject: PassthroughSubject<CurrencyRates, ResourceError> { get }
  
  func getCurrency()
}
