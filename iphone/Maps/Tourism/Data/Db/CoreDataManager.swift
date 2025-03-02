import CoreData

final class CoreDataManager: NSObject {
    
    static let shared = CoreDataManager()
    
  
    let persistentContainer: NSPersistentContainer
  
    var viewContext: NSManagedObjectContext {
        return persistentContainer.viewContext
    }
    
    var backgroundContext: NSManagedObjectContext {
        return persistentContainer.newBackgroundContext()
    }
  
  
    private override init() {
      persistentContainer = NSPersistentContainer(name: "TourismDB")
      persistentContainer.loadPersistentStores { (storeDescription, error) in
          if let error = error as NSError? {
              fatalError("Unresolved error \(error), \(error.userInfo)")
          }
      }
      
      persistentContainer.viewContext.automaticallyMergesChangesFromParent = true
      
    }
  
//    lazy var persistentContainer: NSPersistentContainer = {
//        let container = NSPersistentContainer(name: "TourismDB")
//        container.loadPersistentStores { (storeDescription, error) in
//            if let error = error as NSError? {
//                fatalError("Unresolved error \(error), \(error.userInfo)")
//            }
//        }
//        return container
//    }()
    

    
    func saveContext() {
        let context = viewContext
        if context.hasChanges {
            do {
                try context.save()
            } catch {
                let nserror = error as NSError
                fatalError("Unresolved error \(nserror), \(nserror.userInfo)")
            }
        }
    }
}
