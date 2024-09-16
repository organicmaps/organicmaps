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
      let favoritesDto = try await placesService.getFavorites()
      
      if(hashes.isEmpty) {
        downloadProgress.send(DownloadProgress.loading)
        let allData = try await placesService.getAllPlaces()
        
        // get data
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
        placesPersistenceController.putPlaces(sights, categoryId: PlaceCategory.sights.id)
        placesPersistenceController.putPlaces(restaurants, categoryId: PlaceCategory.restaurants.id)
        placesPersistenceController.putPlaces(hotels, categoryId: PlaceCategory.hotels.id)
        
        // update reviews
        reviewsPersistenceController.deleteAllReviews()
        reviewsPersistenceController.putReviews(reviews)
        
        // update favorites
        favorites.forEach { favorite in
          placesPersistenceController.setFavorite(placeId: favorite.id, isFavorite: favorite.isFavorite)
        }
        
        // update hashes
        hashesPersistenceController.putHashes(hashes: [
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
      
      placesPersistenceController.putPlaces(places, categoryId: categoryId)
      
      // update reviews
      var reviews = [Review]()
      places.forEach { place in
        reviews.append(contentsOf: place.reviews ?? [])
      }
      
      reviewsPersistenceController.deleteAllReviews()
      reviewsPersistenceController.putReviews(reviews)
      
      // update hash
      hashesPersistenceController.putHash(
        Hash(categoryId: hash!.categoryId, value: resource.hash),
        shouldSave: true
      )
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
    
    placesPersistenceController.addFavoriteSync(
      placeId: placeId,
      isFavorite: isFavorite
    )
    
    Task {
      let favoritesIdsDto = FavoritesIdsDTO(marks: [placeId])
      
      do {
        if(isFavorite) {
          try await placesService.addFavorites(ids: favoritesIdsDto)
        } else {
          try await placesService.removeFromFavorites(ids: favoritesIdsDto)
        }
        
        placesPersistenceController.removeFavoriteSync(placeIds: [placeId])
      } catch {
        placesPersistenceController.addFavoriteSync(placeId: placeId, isFavorite: isFavorite)
      }
    }
  }
  
  func syncFavorites() {
    // TODO: cmon
  }
}
