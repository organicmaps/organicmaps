import CoreData
import Combine

class CurrencyPersistenceController {
    static let shared = CurrencyPersistenceController()
    private let coreDataController: SingleEntityCoreDataController<CurrencyRatesEntity>
    
    private init() {
        coreDataController = SingleEntityCoreDataController()
    }
    
    var currencyRatesSubject: PassthroughSubject<CurrencyRatesEntity?, ResourceError> {
        return coreDataController.entitySubject
    }
    
    func observeCurrencyRates() {
        let fetchRequest: NSFetchRequest<CurrencyRatesEntity> = CurrencyRatesEntity.fetchRequest()
        let sortDescriptor = NSSortDescriptor(key: "id", ascending: true)
        coreDataController.observeEntity(fetchRequest: fetchRequest, sortDescriptor: sortDescriptor)
    }
    
    func updateCurrencyRates(entity: CurrencyRates) -> AnyPublisher<Void, ResourceError> {
        let fetchRequest: NSFetchRequest<CurrencyRatesEntity> = CurrencyRatesEntity.fetchRequest()
        return coreDataController.updateEntity(updateBlock: { entityToUpdate in
            entityToUpdate.usd = entity.usd
            entityToUpdate.eur = entity.eur
            entityToUpdate.rub = entity.rub
        }, fetchRequest: fetchRequest)
    }
}
