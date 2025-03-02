import Foundation
import CoreData

class HashesPersistenceController {
  static let shared = HashesPersistenceController()
  
  private let viewContext = CoreDataManager.shared.viewContext
  
  // MARK: - CRUD Operations
  func insertHashes(hashes: [Hash]) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      do {
        for hash in hashes {
          let newHash = HashEntity(context: backgroundContext)
          newHash.categoryId = hash.categoryId
          newHash.value = hash.value
        }
        try backgroundContext.save()
      } catch {
        print("Failed to save context: \(error)")
      }
    }
    
  }
  
  func getHash(categoryId: Int64) -> Hash? {
    let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", categoryId)
    fetchRequest.fetchLimit = 1
    
    do {
      let result = try viewContext.fetch(fetchRequest).first
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
    let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
    
    do {
      let result = try viewContext.fetch(fetchRequest)
      let hashes = result.map { hashEntity in
        Hash(categoryId: hashEntity.categoryId, value: hashEntity.value!)
      }
      return hashes
    } catch {
      print("Failed to fetch hashes: \(error)")
      return []
    }
  }
  
  func deleteHash(hash: Hash) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<HashEntity> = HashEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", hash.categoryId)
      
      do {
        if let hash = try backgroundContext.fetch(fetchRequest).first {
          backgroundContext.delete(hash)
          try backgroundContext.save()
        }
      } catch {
        print(error)
        print("Failed to delete review: \(error)")
      }
    }
  }
  
}
