import CoreData
import Combine

class SingleEntityCoreDataController<Entity: NSManagedObject>: NSObject, NSFetchedResultsControllerDelegate {
  
  private var fetchedResultsController: NSFetchedResultsController<Entity>?
  var entitySubject = PassthroughSubject<Entity?, ResourceError>()
  
  private let viewContext = CoreDataManager.shared.viewContext
  private let backgroundContext = CoreDataManager.shared.backgroundContext
  
//  init(modelName: String) {
//    container = NSPersistentContainer(name: modelName)
//    super.init()
//    container.loadPersistentStores { (description, error) in
//      if let error = error {
//        fatalError("Failed to load Core Data stack: \(error)")
//      }
//    }
//    container.viewContext.automaticallyMergesChangesFromParent = true
//  }
  
//  var context: NSManagedObjectContext {
//    return container.viewContext
//  }
  
  func observeEntity(fetchRequest: NSFetchRequest<Entity>, sortDescriptor: NSSortDescriptor) {
    fetchRequest.sortDescriptors = [sortDescriptor]
    
    fetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: viewContext,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
    
    fetchedResultsController?.delegate = self
    
    do {
      try fetchedResultsController?.performFetch()
      if let fetchedEntity = fetchedResultsController?.fetchedObjects?.first {
        entitySubject.send(fetchedEntity)
      } else {
        entitySubject.send(nil)
      }
    } catch {
      entitySubject.send(completion: .failure(ResourceError.cacheError))
    }
  }
  
  func updateEntity(updateBlock: @escaping (Entity) -> Void, fetchRequest: NSFetchRequest<Entity>) -> AnyPublisher<Void, ResourceError> {
    Future { promise in
      do {
        let entityToUpdate = try self.viewContext.fetch(fetchRequest).first ?? Entity(context: self.viewContext)
        updateBlock(entityToUpdate)
        try self.viewContext.save()
        
        promise(.success(()))
      } catch {
        promise(.failure(ResourceError.cacheError))
      }
    }
    .eraseToAnyPublisher()
  }
  
  
  // NSFetchedResultsControllerDelegate
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    guard let fetchedObjects = controller.fetchedObjects as? [Entity],
          let updatedEntity = fetchedObjects.first else {
      entitySubject.send(completion: .failure(ResourceError.cacheError))
      return
    }
    entitySubject.send(updatedEntity)
  }
}
