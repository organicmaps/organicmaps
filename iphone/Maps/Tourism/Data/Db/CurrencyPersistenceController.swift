import CoreData
import Combine

class CurrencyPersistenceController: NSObject {
  static let shared = CurrencyPersistenceController()
  
  let container: NSPersistentContainer
  
  private var currencyRatesSubject = PassthroughSubject<CurrencyRatesEntity?, ResourceError>()
  
  private override init() {
    container = NSPersistentContainer(name: "Currency")
    super.init()
    container.loadPersistentStores { (description, error) in
      if let error = error {
        fatalError("Failed to load Core Data stack: \(error)")
      }
    }
  }
  
  var context: NSManagedObjectContext {
    return container.viewContext
  }
  
  func observeCurrencyRates() -> AnyPublisher<CurrencyRatesEntity?, ResourceError> {
    // Use NSFetchedResultsController to observe changes
    let fetchRequest: NSFetchRequest<CurrencyRatesEntity> = CurrencyRatesEntity.fetchRequest()
    
    let fetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
    
    fetchedResultsController.delegate = self
    
    do {
      try fetchedResultsController.performFetch()
      if let fetchedEntity = fetchedResultsController.fetchedObjects?.first {
        currencyRatesSubject.send(fetchedEntity)
      } else {
        debugPrint("No data")
        currencyRatesSubject.send(completion: .failure(ResourceError.cacheError))
      }
    } catch {
      debugPrint("Failed to fetch initial data: \(error)")
      currencyRatesSubject.send(completion: .failure(ResourceError.cacheError))
    }
    
    return currencyRatesSubject.eraseToAnyPublisher()
  }
  
  func updateCurrencyRates(entity: CurrencyRates) -> AnyPublisher<Void, ResourceError> {
    Future { promise in
      let fetchRequest: NSFetchRequest<CurrencyRatesEntity> = CurrencyRatesEntity.fetchRequest()
      do {
        let entityToUpdate = try self.context.fetch(fetchRequest).first ?? CurrencyRatesEntity(context: self.context)
        entityToUpdate.usd = entity.usd
        entityToUpdate.eur = entity.eur
        entityToUpdate.rub = entity.rub
        try self.context.save()
        
        promise(.success(()))
      } catch {
        promise(.failure(ResourceError.cacheError))
      }
    }
    .eraseToAnyPublisher()
  }
}

extension CurrencyPersistenceController: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    guard let fetchedObjects = controller.fetchedObjects as? [CurrencyRatesEntity],
          let updatedEntity = fetchedObjects.first else {
      currencyRatesSubject.send(nil)
      return
    }
    
    // Emit the updated entity through the Combine subject
    currencyRatesSubject.send(updatedEntity)
  }
}
