import Combine

class PlacesRepositoryImpl: PlacesRepository {
  let placesService: PlacesService
  let placesPersistenceController: PlacesPersistenceController
  let reviewsPersistenceController: ReviewsPersistenceController
  let hashesPersistenceController: HashesPersistenceController
  
  var downloadProgress: PassthroughSubject<DownloadProgress, ResourceError>
  var searchResource: PassthroughSubject<[PlaceShort], ResourceError>
  var placesByCategoryResource: PassthroughSubject<[PlaceShort], ResourceError>
  var topSightsResource: PassthroughSubject<[PlaceShort], ResourceError>
  var topRestaurantsResource: PassthroughSubject<[PlaceShort], ResourceError>
  var placeResource: PassthroughSubject<PlaceFull, ResourceError>
  var favoritesResource: PassthroughSubject<[PlaceShort], ResourceError>
  
  
  init(
    placesService: PlacesService,
    placesPersistenceController: PlacesPersistenceController,
    reviewsPersistenceController: ReviewsPersistenceController,
    hashesPersistenceController: HashesPersistenceController
  ) {
    self.placesService = placesService
    self.placesPersistenceController = placesPersistenceController
    self.hashesPersistenceController = hashesPersistenceController
    self.reviewsPersistenceController = reviewsPersistenceController
    
    downloadProgress = PassthroughSubject<DownloadProgress, ResourceError>()
    downloadProgress.send(DownloadProgress.idle)
    searchResource = placesPersistenceController.searchSubject
    placesByCategoryResource = placesPersistenceController.placesByCatSubject
    topSightsResource = placesPersistenceController.topSightsSubject
    topRestaurantsResource = placesPersistenceController.topRestaurantsSubject
    placeResource = placesPersistenceController.singlePlaceSubject
    favoritesResource = placesPersistenceController.favoritePlacesSubject
  }
  
  func downloadAllData() async throws {
    do {
      let hashes = hashesPersistenceController.getHashes()
      
      if(hashes.isEmpty) {
        downloadProgress.send(DownloadProgress.loading)
        
        // download all data
        let allData = try await placesService.getAllPlaces()
        let favoritesDto = try await placesService.getFavorites()
        
        // patch data
        let favorites = favoritesDto.data.map { placeDto in
          placeDto.toPlaceFull(isFavorite: true)
        }
        
        var reviews: [Review] = []
        
        func toPlaceFull(placeDto: PlaceDTO) -> PlaceFull {
          var placeFull = placeDto.toPlaceFull(isFavorite: false)
          
          placeFull.isFavorite = favorites.contains { $0.id == placeFull.id }
          
          if let placeReviews = placeFull.reviews {
            reviews.append(contentsOf: placeReviews)
          }
          
          return placeFull
        }
        let sights = allData.attractions.map { placeDto in
          toPlaceFull(placeDto: placeDto)
        }
        let restaurants = allData.restaurants.map { placeDto in
          toPlaceFull(placeDto: placeDto)
        }
        let hotels = allData.accommodations.map { placeDto in
          toPlaceFull(placeDto: placeDto)
        }
        
        // update places
        placesPersistenceController.deleteAllPlaces()
        placesPersistenceController.insertPlaces(sights, categoryId: PlaceCategory.sights.id)
        placesPersistenceController.insertPlaces(restaurants, categoryId: PlaceCategory.restaurants.id)
        placesPersistenceController.insertPlaces(hotels, categoryId: PlaceCategory.hotels.id)
        
        // update reviews
        reviewsPersistenceController.deleteAllReviews()
        reviewsPersistenceController.insertReviews(reviews)
        
        // update favorites
        favorites.forEach { favorite in
          placesPersistenceController.setFavorite(placeId: favorite.id, isFavorite: favorite.isFavorite)
        }
        
        // update hashes
        hashesPersistenceController.insertHashes(hashes: [
          Hash(categoryId: PlaceCategory.sights.id, value: allData.attractionsHash),
          Hash(categoryId: PlaceCategory.restaurants.id, value: allData.restaurantsHash),
          Hash(categoryId: PlaceCategory.hotels.id, value: allData.accommodationsHash)
        ])
        
        // return response
        downloadProgress.send(DownloadProgress.success)
      } else {
        downloadProgress.send(DownloadProgress.success)
      }
    } catch let error as ResourceError {
      downloadProgress.send(completion: .failure(error))
    }
  }
  
  func observeSearch(query: String) {
    placesPersistenceController.observeSearch(query: query)
  }
  
  func observePlacesByCategoryAndUpdate(categoryId: Int64) {
    placesPersistenceController.observePlacesByCategoryId(categoryId: categoryId)
    Task {
      try await getPlacesByCategoryFromApiIfThereIsChange(categoryId)
    }
  }
  
  func observeTopSightsAndUpdate() {
    placesPersistenceController.observeTopSights()
    Task {
      try await getPlacesByCategoryFromApiIfThereIsChange(PlaceCategory.sights.id)
    }
  }
  
  func observeTopRestaurantsAndUpdate() {
    placesPersistenceController.observeTopRestaurants()
    Task {
      try await getPlacesByCategoryFromApiIfThereIsChange(PlaceCategory.restaurants.id)
    }
  }
  
  private func getPlacesByCategoryFromApiIfThereIsChange(
    _ categoryId: Int64
  ) async throws -> Void {
    let hash = hashesPersistenceController.getHash(categoryId: categoryId)
    
    let favorites = placesPersistenceController.getFavoritePlaces()
    
    let resource =
    try await placesService.getPlacesByCategory(id: categoryId, hash: hash?.value ?? "")
    
    let places = resource.data
    if (hash != nil && !places.isEmpty) {
      // update places
      placesPersistenceController.deleteAllPlacesByCategory(categoryId: categoryId)
      
      let places = places.map { placeDto in
        var placeFull = placeDto.toPlaceFull(isFavorite: false)
        placeFull.isFavorite = favorites.contains { $0.id == placeFull.id }
        return placeFull
      }
      
      placesPersistenceController.insertPlaces(places, categoryId: categoryId)
      
      // update reviews
      places.forEach { place in
        reviewsPersistenceController.deleteAllPlaceReviews(placeId: place.id)
        reviewsPersistenceController.insertReviews(place.reviews ?? [])
      }
      
      // update hash
      hashesPersistenceController.deleteHash(hash: hash!)
      hashesPersistenceController.insertHashes(hashes: [
        Hash(categoryId: hash!.categoryId, value: resource.hash)
      ])
    }
  }
  
  func observePlaceById(_ id: Int64) {
    placesPersistenceController.observePlaceById(placeId: id)
  }
  
  func observeFavorites(query: String) {
    placesPersistenceController.observeFavoritePlaces(query: query)
  }
  
  func setFavorite(placeId: Int64, isFavorite: Bool) {
    placesPersistenceController.setFavorite(placeId: placeId, isFavorite: isFavorite)
    
    placesPersistenceController.addFavoritingRecordForSync(
      placeId: placeId,
      isFavorite: isFavorite
    )
    
    Task {
      let favoritesIdsDto = FavoritesIdsDTO(marks: [placeId])
      
      do {
        if(isFavorite) {
          let _ = try await placesService.addFavorites(ids: favoritesIdsDto)
        } else {
          let _ = try await placesService.removeFromFavorites(ids: favoritesIdsDto)
        }
        
        placesPersistenceController.removeFavoritingRecordsForSync(placeIds: [placeId])
      } catch {
        print("Failed to setFavorite")
        print(error)
      }
    }
  }
  
  func syncFavorites() {
    let syncData = placesPersistenceController.getFavoritingRecordsForSync()
    
    let favoritesToAdd = syncData.filter { $0.isFavorite }.map { $0.placeId }
    let favoritesToRemove = syncData.filter { !$0.isFavorite }.map { $0.placeId }
    
    if !favoritesToAdd.isEmpty {
      Task {
        do {
          _ =
          try await placesService.addFavorites(ids: FavoritesIdsDTO(marks: favoritesToAdd))
          placesPersistenceController.removeFavoritingRecordsForSync(placeIds: favoritesToAdd)
        } catch {
          print(error)
        }
      }
    }
    
    if !favoritesToRemove.isEmpty {
      Task {
        do {
          _ =
          try await placesService.removeFromFavorites(ids: FavoritesIdsDTO(marks: favoritesToRemove))
          placesPersistenceController.removeFavoritingRecordsForSync(placeIds: favoritesToRemove)
        } catch {
          print(error)
        }
      }
    }
  }
}
