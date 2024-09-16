import Foundation
import CoreData

class HashesPersistenceController {
  static let shared = HashesPersistenceController()
  
  let container: NSPersistentContainer
  
  init(inMemory: Bool = false) {
    container = NSPersistentContainer(name: "Place")
    if inMemory {
      container.persistentStoreDescriptions.first!.url = URL(fileURLWithPath: "/dev/null")
    }
    container.loadPersistentStores { (storeDescription, error) in
      if let error = error as NSError? {
        fatalError("Unresolved error \(error), \(error.userInfo)")
      }
    }
  }
  
  // MARK: - CRUD Operations
  func putOneHash(_ hash: Hash) {
    putHash(hash, shouldSave: true)
  }
  
  func putHashes(hashes: [Hash]) {
    hashes.forEach { hash in
      putHash(hash, shouldSave: false)  // Don't save in each iteration
    }
    
    // Save the context once after all inserts/updates
    let context = container.viewContext
    do {
      try context.save()
    } catch {
      print("Failed to save context: \(error)")
    }
  }
  
  func putHash(_ hash: Hash, shouldSave: Bool) {
    let context = container.viewContext
    let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", hash.categoryId)
    fetchRequest.fetchLimit = 1
    
    do {
      let result = try context.fetch(fetchRequest).first
      
      if let existingHash = result {
        existingHash.value = hash.value
      } else {
        let newHash = HashEntity(context: context)
        newHash.categoryId = hash.categoryId
        newHash.value = hash.value
      }
      
      if shouldSave {
        try context.save()
      }
    } catch {
      print("Failed to insert or update hash: \(error)")
    }
  }
  
  func getHash(categoryId: Int64) -> Hash? {
    let context = container.viewContext
    let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", categoryId)
    fetchRequest.fetchLimit = 1
    
    do {
      let result = try context.fetch(fetchRequest).first
      if let result = result {
        return Hash(categoryId: result.categoryId, value: result.value!)
      } else {
        return nil
      }
    } catch {
      print("Failed to fetch hash: \(error)")
      return nil
    }
  }
  
  func getHashes() -> [Hash] {
    let context = container.viewContext
    let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
    
    do {
      let result = try context.fetch(fetchRequest)
      let hashes = result.map { hashEntity in
        Hash(categoryId: hashEntity.categoryId, value: hashEntity.value!)
      }
      return hashes
    } catch {
      print("Failed to fetch hashes: \(error)")
      return []
    }
  }
}
