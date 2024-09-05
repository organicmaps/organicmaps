import Combine

protocol PlacesRepository {
  func downloadAllData() -> AnyPublisher<SimpleResponse, ResourceError>
  
  func search(query: String) -> AnyPublisher<[PlaceShort], ResourceError>
  
  var placesByCategoryResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func getPlacesByCategoryAndUpdate(id: Int64)
  
  var topPlacesResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func getTopPlaces(is: Int64)
  
  var placeResource: PassthroughSubject<PlaceFull, ResourceError> { get }
  func getPlaceById(is: Int64)
  
  var favoritesResource: PassthroughSubject<[PlaceShort], ResourceError> { get }
  func getFavorites(query: String)
  
  func setFavorite(placeId: Int64, isFavorite: Bool)
  
  func syncFavorites()
}
