import Combine

protocol PlacesRepository {
  var downloadProgress: PassthroughSubject<DownloadProgress, ResourceError> { get }
  func downloadAllData() async throws
  
  var searchResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func observeSearch(query: String)
  
  var placesByCategoryResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func observePlacesByCategoryAndUpdate(categoryId: Int64)
  
  var topSightsResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func observeTopSightsAndUpdate()
  
  var topRestaurantsResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func observeTopRestaurantsAndUpdate()
  
  var placeResource: PassthroughSubject<PlaceFull, ResourceError> { get }
  func observePlaceById(_ id: Int64)
  
  var favoritesResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func observeFavorites(query: String)
  
  func setFavorite(placeId: Int64, isFavorite: Bool)
  
  func syncFavorites()
}
