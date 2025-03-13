import CoreData
import Combine

class PlacesPersistenceController: NSObject, NSFetchedResultsControllerDelegate {
  static let shared = PlacesPersistenceController()
  
  private let viewContext = CoreDataManager.shared.viewContext
  
  private var searchFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  private var placesByCatFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  private var topSightsFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  private var topRestaurantsFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  private var singlePlaceFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  private var favoritePlacesFetchedResultsController: NSFetchedResultsController<PlaceEntity>?
  
  let searchSubject = PassthroughSubject<[PlaceShort], ResourceError>()
  let placesByCatSubject = PassthroughSubject<[PlaceShort], ResourceError>()
  let topSightsSubject = PassthroughSubject<[PlaceShort], ResourceError>()
  let topRestaurantsSubject = PassthroughSubject<[PlaceShort], ResourceError>()
  let singlePlaceSubject = PassthroughSubject<PlaceFull, ResourceError>()
  let favoritePlacesSubject = PassthroughSubject<[PlaceShort], ResourceError>()
  
  
//  init(inMemory: Bool = false) {
//    container = NSPersistentContainer(name: "Place")
//    if inMemory {
//      container.persistentStoreDescriptions.first!.url = URL(fileURLWithPath: "/dev/null")
//    }
//    super.init()
//    container.loadPersistentStores { (storeDescription, error) in
//      if let error = error as NSError? {
//        fatalError("Unresolved error \(error), \(error.userInfo)")
//      }
//    }
//    container.viewContext.automaticallyMergesChangesFromParent = true
//  }
  
  // MARK: Places
  func insertPlaces(_ places: [PlaceFull], categoryId: Int64) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform { [weak self] in
      do {
        for place in places {
          let newPlace = PlaceEntity(context: backgroundContext)
          newPlace.id = place.id
          self?.updatePlace(newPlace, with: place, categoryId: categoryId)
        }
        try backgroundContext.save()
      } catch {
        print("Failed to save context: \(error)")
      }
    }
    
  }
  
  private func updatePlace(_ placeEntity: PlaceEntity, with place: PlaceFull, categoryId: Int64) {
      placeEntity.categoryId = categoryId
      placeEntity.name = place.name
      placeEntity.excerpt = place.excerpt
      placeEntity.descr = place.description
      placeEntity.cover = place.cover
      placeEntity.galleryJson = DBUtils.encodeToJsonString(place.pics)
      placeEntity.coordinatesJson = DBUtils.encodeToJsonString(place.placeLocation?.toCoordinatesEntity())
      placeEntity.rating = place.rating
      placeEntity.isFavorite = place.isFavorite
  }
  
  func deleteAllPlaces() {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = PlaceEntity.fetchRequest()
      let deleteRequest = NSBatchDeleteRequest(fetchRequest: fetchRequest)
      
      // Configure the request to return the IDs of the deleted objects
      deleteRequest.resultType = .resultTypeObjectIDs
      
      do {
        let result = try backgroundContext.execute(deleteRequest) as? NSBatchDeleteResult
        let changes: [AnyHashable: Any] = [
          NSDeletedObjectsKey: result?.result as? [NSManagedObjectID] ?? []
        ]
        
        NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: [backgroundContext])
        try backgroundContext.save()
      } catch {
        print("Failed to delete all places: \(error)")
      }
    }
    
  }
  
  func deleteAllPlacesByCategory(categoryId: Int64) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = PlaceEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "categoryId == %d", categoryId)
      
      let deleteRequest = NSBatchDeleteRequest(fetchRequest: fetchRequest)
      
      // Configure the request to return the IDs of the deleted objects
      deleteRequest.resultType = .resultTypeObjectIDs
      
      do {
        let result = try backgroundContext.execute(deleteRequest) as? NSBatchDeleteResult
        let changes: [AnyHashable: Any] = [
          NSDeletedObjectsKey: result?.result as? [NSManagedObjectID] ?? []
        ]
        
        NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: [backgroundContext])
        try backgroundContext.save()
      } catch {
        print("Failed to delete places by category \(categoryId): \(error)")
      }
    }
    
  }
  
  // MARK: Observe places
  func observeSearch(query: String) {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "id", ascending: true)]
    if !query.isEmpty {
      fetchRequest.predicate = NSPredicate(format: "name CONTAINS[cd] %@", query)
    }
    
    searchFetchedResultsController = NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil)
    
    searchFetchedResultsController?.delegate = self
    
    do {
      try searchFetchedResultsController!.performFetch()
      if let results = searchFetchedResultsController!.fetchedObjects {
        searchSubject.send(results.toShortPlaces())
      }
    } catch {
      searchSubject.send(completion: .failure(.cacheError))
    }
  }
  
  func observePlacesByCategoryId(categoryId: Int64) {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "id", ascending: true)]
    fetchRequest.predicate = NSPredicate(format: "categoryId == %d", categoryId)
    
    placesByCatFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil
    )
    
    placesByCatFetchedResultsController?.delegate = self
    
    do {
      try placesByCatFetchedResultsController!.performFetch()
      if let results = placesByCatFetchedResultsController!.fetchedObjects {
        placesByCatSubject.send(results.toShortPlaces())
      }
    } catch {
      placesByCatSubject.send(completion: .failure(.cacheError))
    }
  }
  
  func observeTopSights() {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", PlaceCategory.sights.id)
    fetchRequest.sortDescriptors = [
      NSSortDescriptor(key: "rating", ascending: false),
      NSSortDescriptor(key: "name", ascending: true)
    ]
    fetchRequest.fetchLimit = 15
    
    topSightsFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil
    )
    
    topSightsFetchedResultsController?.delegate = self
    
    do {
      try topSightsFetchedResultsController!.performFetch()
      if let results = topSightsFetchedResultsController!.fetchedObjects {
        topSightsSubject.send(results.toShortPlaces())
      }
    } catch {
      topSightsSubject.send(completion: .failure(.cacheError))
    }
  }  
  
  func observeTopRestaurants() {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "categoryId == %lld", PlaceCategory.restaurants.id)
    fetchRequest.sortDescriptors = [
      NSSortDescriptor(key: "rating", ascending: false),
      NSSortDescriptor(key: "name", ascending: true)
    ]
    fetchRequest.fetchLimit = 15
    
    topRestaurantsFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil
    )
    
    topRestaurantsFetchedResultsController?.delegate = self
    
    do {
      try topRestaurantsFetchedResultsController!.performFetch()
      if let results = topRestaurantsFetchedResultsController!.fetchedObjects {
        topRestaurantsSubject.send(results.toShortPlaces())
      }
    } catch {
      topRestaurantsSubject.send(completion: .failure(.cacheError))
    }
  }
  
  func observePlaceById(placeId: Int64) {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.predicate = NSPredicate(format: "id == %lld", placeId)
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "id", ascending: true)]
    fetchRequest.fetchLimit = 1
    
    singlePlaceFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil
    )
    
    singlePlaceFetchedResultsController?.delegate = self
    
    do {
      try singlePlaceFetchedResultsController!.performFetch()
      if let results = singlePlaceFetchedResultsController!.fetchedObjects {
        if let place = results.first {
          singlePlaceSubject.send(place.toPlaceFull())
        }
      }
    } catch {
      singlePlaceSubject.send(completion: .failure(.cacheError))
    }
  }
  
  // MARK: Favorites
  func observeFavoritePlaces(query: String = "") {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "id", ascending: true)]
    var predicates = [
      NSPredicate(format: "isFavorite == YES"),
    ]
    if !query.isEmpty {
      predicates.append(NSPredicate(format: "name CONTAINS[cd] %@", query))
    }
    fetchRequest.predicate = NSCompoundPredicate(andPredicateWithSubpredicates: predicates)
    
    favoritePlacesFetchedResultsController = NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: viewContext, sectionNameKeyPath: nil, cacheName: nil
    )
    
    favoritePlacesFetchedResultsController?.delegate = self
    
    do {
      try favoritePlacesFetchedResultsController!.performFetch()
      if let results = favoritePlacesFetchedResultsController!.fetchedObjects {
        favoritePlacesSubject.send(results.toShortPlaces())
      }
    } catch {
      favoritePlacesSubject.send(completion: .failure(.cacheError))
    }
  }
  
  func getFavoritePlaces(query: String = "") -> [PlaceEntity] {
    let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "name", ascending: true)]
    var predicates = [
      NSPredicate(format: "isFavorite == YES"),
    ]
    if !query.isEmpty {
      predicates.append(NSPredicate(format: "name CONTAINS[cd] %@", query))
    }
    fetchRequest.predicate = NSCompoundPredicate(andPredicateWithSubpredicates: predicates)
    
    do {
      return try viewContext.fetch(fetchRequest)
    } catch {
      print("Failed to fetch favorite places: \(error)")
      return []
    }
  }
  
  func setFavorite(placeId: Int64, isFavorite: Bool) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<PlaceEntity> = PlaceEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "id == %lld", placeId)
      
      do {
        if let place = try backgroundContext.fetch(fetchRequest).first {
          place.isFavorite = isFavorite
          try backgroundContext.save()
        }
      } catch {
        print("Failed to set favorite status: \(error)")
      }
    }
    
  }
  
  func addFavoritingRecordForSync(placeId: Int64, isFavorite: Bool) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let favoriteSyncEntity = FavoriteSyncEntity(context: backgroundContext)
      favoriteSyncEntity.placeId = placeId
      favoriteSyncEntity.isFavorite = isFavorite
      
      do {
        try backgroundContext.save()
      } catch {
        print("Failed to add favorite sync: \(error)")
      }
    }
    
  }
  
  func removeFavoritingRecordsForSync(placeIds: [Int64]) {
    
    let backgroundContext = CoreDataManager.shared.backgroundContext
    
    backgroundContext.perform {
      let fetchRequest: NSFetchRequest<NSFetchRequestResult> = FavoriteSyncEntity.fetchRequest()
      fetchRequest.predicate = NSPredicate(format: "placeId IN %@", placeIds)
      
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
        print("Failed to remove favoriting records for sync: \(error)")
      }
    }
    
  }
  
  func getFavoritingRecordsForSync() -> [FavoriteSyncEntity] {
    let fetchRequest: NSFetchRequest<FavoriteSyncEntity> = FavoriteSyncEntity.fetchRequest()
    
    do {
      return try viewContext.fetch(fetchRequest)
    } catch {
      print("Failed to fetch favorite sync data: \(error)")
      return []
    }
  }
  
  // MARK: - NSFetchedResultsControllerDelegate
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    guard let fetchedObjects = controller.fetchedObjects as? [PlaceEntity] else {
      return
    }
    
    switch controller {
    case searchFetchedResultsController:
      searchSubject.send(fetchedObjects.toShortPlaces())
    case placesByCatFetchedResultsController:
      placesByCatSubject.send(fetchedObjects.toShortPlaces())
    case topSightsFetchedResultsController:
      topSightsSubject.send(fetchedObjects.toShortPlaces())
    case topRestaurantsFetchedResultsController:
      topRestaurantsSubject.send(fetchedObjects.toShortPlaces())
    case singlePlaceFetchedResultsController:
      if let place = fetchedObjects.first {
        singlePlaceSubject.send(place.toPlaceFull())
      }
    case favoritePlacesFetchedResultsController:
      favoritePlacesSubject.send(fetchedObjects.toShortPlaces())
    default:
      break
    }
  }
}
