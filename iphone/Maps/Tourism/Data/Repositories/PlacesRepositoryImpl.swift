import Combine

class PlacesRepositoryImpl: PlacesRepository {
  func downloadAllData() -> AnyPublisher<SimpleResponse, ResourceError> {
    // TODO: cmon
    return PassthroughSubject<SimpleResponse, ResourceError>().eraseToAnyPublisher()
  }
  
  func search(query: String) -> AnyPublisher<[PlaceShort], ResourceError> {
    // TODO: cmon
    return PassthroughSubject<[PlaceShort], ResourceError>().eraseToAnyPublisher()
  }
  
  var placesByCategoryResource = PassthroughSubject<[PlaceShort], ResourceError>()
  func getPlacesByCategoryAndUpdate(id: Int64) {
    // TODO: cmon
  }
  
  var topPlacesResource = PassthroughSubject<[PlaceShort], ResourceError>()
  func getTopPlaces(is: Int64) {
    // TODO: cmon
  }
  
  var placeResource = PassthroughSubject<PlaceFull, ResourceError>()
  func getPlaceById(is: Int64) {
    // TODO: cmon
  }
  
  var favoritesResource = PassthroughSubject<[PlaceShort], ResourceError>()
  func getFavorites(query: String) {
    // TODO: cmon
  }
  
  func setFavorite(placeId: Int64, isFavorite: Bool) {
    // TODO: cmon
  }
  
  func syncFavorites() {
    // TODO: cmon
  }
}
