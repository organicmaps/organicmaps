import Foundation
import CoreData
import Combine

class ReviewsPersistenceController: NSObject, NSFetchedResultsControllerDelegate {
  static let shared = ReviewsPersistenceController()
  
  private let viewContext = CoreDataManager.shared.viewContext
  
  private var reviewsForPlaceFetchedResultsController: NSFetchedResultsController<ReviewEntity>?
  private var reviewsPlannedToPostFetchedResultsController: NSFetchedResultsController<ReviewPlannedToPostEntity>?
  
  let reviewsForPlaceSubject = PassthroughSubject<[Review], ResourceError>()
  let reviewsPlannedToPostSubject = PassthroughSubject<[ReviewPlannedToPostEntity], ResourceError>()
  
//  override init() {
//    container = NSPersistentContainer(name: "Place")
//    super.init()
//    container.loadPersistentStores { (storeDescription, error) in
//      if let error = error as NSError? {
//        fatalError("Unresolved error \(error), \(error.userInfo)")
//      }
//    }
//    container.viewContext.automaticallyMergesChangesFromParent = true
//  }
  
  // MARK: - Review Operations
  func insertReviews(_ reviews: [Review]) {
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      do {
        for review in reviews {
          let newReview = ReviewEntity(context: backgroundContext)
          newReview.id = review.id
          self.updateReviewEntity(newReview, with: review)
        }
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to insert/update reviews: \(error)")
      }
    }
    
    
  }
  
  private func updateReviewEntity(_ entity: ReviewEntity, with review: Review) {
    entity.placeId = review.placeId
    entity.rating = Int16(review.rating)
    entity.userJson = DBUtils.encodeToJsonString(review.user?.toUserEntity())
    entity.date = review.date
    entity.comment = review.comment
    entity.picsUrlsJson = DBUtils.encodeToJsonString(review.picsUrls)
    entity.deletionPlanned = review.deletionPlanned
  }
  
  func deleteReview(id: Int64) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<ReviewEntity> = ReviewEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "id == %lld", id)
      
      do {
        let reviews = try backgroundContext.fetch(fetchRequest)
        for review in reviews {
          backgroundContext.delete(review)
        }
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to delete review: \(error)")
      }
    }
    
  }
  
  func deleteReviews(ids: [Int64]) {
  
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<ReviewEntity> = ReviewEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "id IN %@", ids)
      
      do {
        let reviews = try backgroundContext.fetch(fetchRequest)
        for review in reviews {
          backgroundContext.delete(review)
        }
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to delete reviews: \(error)")
      }
    }
    
  }
  
  func deleteAllReviews() {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = ReviewEntity.fetchRequest()
      let deleteRequest = NSBatchDeleteRequest(fetchRequest: fetchRequest)
      deleteRequest.resultType = .resultTypeObjectIDs
      
      do {
        let result = try backgroundContext.execute(deleteRequest) as? NSBatchDeleteResult
        let changes: [AnyHashable: Any] = [
          NSDeletedObjectsKey: result?.result as? [NSManagedObjectID] ?? []
        ]
        
        NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: [backgroundContext])
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to delete all places: \(error)")
      }
    }
    
  }
  
  func deleteAllPlaceReviews(placeId: Int64) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = ReviewEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "placeId == %lld", placeId)
      let deleteRequest = NSBatchDeleteRequest(fetchRequest: fetchRequest)
      deleteRequest.resultType = .resultTypeObjectIDs
      
      do {
        let result = try backgroundContext.execute(deleteRequest) as? NSBatchDeleteResult
        let changes: [AnyHashable: Any] = [
          NSDeletedObjectsKey: result?.result as? [NSManagedObjectID] ?? []
        ]
        
        NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: [backgroundContext])
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to delete place reviews: \(error)")
      }
    }
    
    
  }
  
  func observeReviewsForPlace(placeId: Int64) {
    
    let fetchRequest: NSFetchRequest<ReviewEntity> = ReviewEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "placeId == %lld", placeId)
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "date", ascending: false)]
    
    reviewsForPlaceFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: viewContext,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
    
    reviewsForPlaceFetchedResultsController?.delegate = self
    
    do {
      try reviewsForPlaceFetchedResultsController?.performFetch()
      if let results = reviewsForPlaceFetchedResultsController?.fetchedObjects {
        reviewsForPlaceSubject.send(results.map({ reviews in
          reviews.toReview()
        }))
      }
    } catch {
      print(error)
      reviewsForPlaceSubject.send(completion: .failure(ResourceError.cacheError))
    }
  }
  
  func markReviewForDeletion(id: Int64, deletionPlanned: Bool = true) {
    let fetchRequest: NSFetchRequest<ReviewEntity> = ReviewEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "id == %lld", id)
    
    do {
      let reviews = try viewContext.fetch(fetchRequest)
      if let review = reviews.first {
        review.deletionPlanned = deletionPlanned
        try viewContext.save()
      }
    } catch {
      print(error)
      print("Failed to mark review for deletion: \(error)")
    }
  }
  
  func getReviewsPlannedForDeletion() -> [ReviewEntity] {
    let fetchRequest: NSFetchRequest<ReviewEntity> = ReviewEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "deletionPlanned == YES")
    
    do {
      return try viewContext.fetch(fetchRequest)
    } catch {
      print(error)
      print("Failed to fetch reviews planned for deletion: \(error)")
      return []
    }
  }
  
  //    // MARK: - Planned Review Operations
  
  func insertReviewPlannedToPost(_ review: ReviewToPost) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let newReview = ReviewPlannedToPostEntity(context: backgroundContext)
      newReview.placeId = review.placeId
      newReview.comment = review.comment
      newReview.rating = Int32(review.rating)
      let imagesJson = DBUtils.encodeToJsonString(review.images)
      newReview.imagesJson = imagesJson
      
      do {
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to insert planned review: \(error)")
      }
    }
    
    
  }
  
  func deleteReviewPlannedToPost(placeId: Int64) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = ReviewPlannedToPostEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "placeId == %lld", placeId)
      
      let deleteRequest = NSBatchDeleteRequest(fetchRequest: fetchRequest)
      deleteRequest.resultType = .resultTypeObjectIDs
      
      do {
        let result = try backgroundContext.execute(deleteRequest) as? NSBatchDeleteResult
        let changes: [AnyHashable: Any] = [
          NSDeletedObjectsKey: result?.result as? [NSManagedObjectID] ?? []
        ]
        
        NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: [backgroundContext])
        try backgroundContext.save()
      } catch {
        print(error)
        print("Failed to delete planned review: \(error)")
      }
    }
    
  }
  
  func getReviewsPlannedToPost() -> [ReviewPlannedToPostEntity] {
    let fetchRequest: NSFetchRequest<ReviewPlannedToPostEntity> = ReviewPlannedToPostEntity.fetchRequest()
    
    do {
      return try viewContext.fetch(fetchRequest)
    } catch {
      print(error)
      print("Failed to fetch planned reviews: \(error)")
      return []
    }
  }
  
  // we only use it to limit the user from reviewing when he already made review
  // for a place
//  func observeReviewsPlannedToPost(placeId: Int64) {
//    let fetchRequest: NSFetchRequest<ReviewPlannedToPostEntity> = ReviewPlannedToPostEntity.fetchRequest()
//    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "placeId", ascending: true)]
//    fetchRequest.predicate = NSPredicate(format: "placeId == %lld", placeId)
//    
//    reviewsPlannedToPostFetchedResultsController = NSFetchedResultsController(
//      fetchRequest: fetchRequest,
//      managedObjectContext: container.viewContext,
//      sectionNameKeyPath: nil,
//      cacheName: nil
//    )
//    
//    reviewsPlannedToPostFetchedResultsController?.delegate = self
//    
//    do {
//      try reviewsPlannedToPostFetchedResultsController?.performFetch()
//      if let results = reviewsPlannedToPostFetchedResultsController?.fetchedObjects {
//        reviewsPlannedToPostSubject.send(results)
//      }
//    } catch {
//      print(error)
//      reviewsPlannedToPostSubject.send(completion: .failure(ResourceError.cacheError))
//    }
//  }
  
  // MARK: - NSFetchedResultsControllerDelegate
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    guard let fetchedObjects = controller.fetchedObjects else {
      return
    }
    
    switch controller {
    case reviewsForPlaceFetchedResultsController:
      let reviewsEntities = fetchedObjects as! [ReviewEntity]
      let reviews = reviewsEntities.map { reviewEntity in reviewEntity.toReview() }
      reviewsForPlaceSubject.send(reviews)
    case reviewsPlannedToPostFetchedResultsController:
      reviewsPlannedToPostSubject.send(fetchedObjects as! [ReviewPlannedToPostEntity])
    default:
      break
    }
  }
}
