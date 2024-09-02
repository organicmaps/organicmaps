import Combine
import Foundation

class CurrencyRepositoryImpl: CurrencyRepository {
  private let currencyService: CurrencyService
  private let persistenceController: CurrencyPersistenceController
  
  let currencyPassThroughSubject = PassthroughSubject<CurrencyRates, ResourceError>()
  
  private var cancellables = Set<AnyCancellable>()
  
  init(currencyService: CurrencyService, currencyPersistenceController: CurrencyPersistenceController) {
    self.currencyService = currencyService
    self.persistenceController = currencyPersistenceController
  }
  
  func getCurrency() {
    // Local persistence subscription
    persistenceController.currencyRatesSubject
      .compactMap { $0?.toCurrencyRates() }
      .sink { completion in
        if case let .failure(error) = completion {
          print(error.localizedDescription)
          self.currencyPassThroughSubject.send(completion: .failure(error))
        }
      } receiveValue: { currencyRates in
        self.currencyPassThroughSubject.send(currencyRates)
      }
      .store(in: &cancellables) // Store the cancellable
    
    persistenceController.observeCurrencyRates()
    
    // Remote service subscription
    currencyService.getCurrencyRates()
      .flatMap { [weak self] remoteRates -> AnyPublisher<CurrencyRates, ResourceError> in
        guard let self = self else {
          print("CurrencyRepositoryImpl/getCurrency/ self was null")
          return Fail(error: ResourceError.other(message: "")).eraseToAnyPublisher()
        }
        
        // Update the local database with the fetched remote rates
        do {
          let newCurrencyRates = try remoteRates.toCurrencyRates()
          return self.persistenceController.updateCurrencyRates(entity: newCurrencyRates)
            .map { newCurrencyRates }
            .eraseToAnyPublisher()
        } catch {
          print("CurrencyRepositoryImpl/getCurrency/ failed to convert dto to domain model")
          return Fail(error: ResourceError.other(message: "")).eraseToAnyPublisher()
        }
      }
      .sink { completion in
        if case let .failure(error) = completion {
          print(error.localizedDescription)
          self.currencyPassThroughSubject.send(completion: .failure(error))
        }
      } receiveValue: { currencyRates in
        // yes, nothing, we observe anyway
      }
      .store(in: &cancellables) // Store the cancellable
  }
}
