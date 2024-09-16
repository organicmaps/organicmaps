import Combine

protocol PlacesService {
  func getPlacesByCategory(id: Int64, hash: String) async throws -> CategoryDTO
  func getAllPlaces() async throws -> AllDataDTO
  func getFavorites() async throws -> FavoritesDTO
  func addFavorites(ids: FavoritesIdsDTO) async throws -> SimpleResponse
  func removeFromFavorites(ids: FavoritesIdsDTO) async throws -> SimpleResponse
}

class PlacesServiceImpl : PlacesService {
  
  func getPlacesByCategory(id: Int64, hash: String) async throws -> CategoryDTO {
    return try await AppNetworkHelper.get(path: APIEndpoints.getPlacesByCategoryUrl(id: id, hash: hash))
  }
  
  func getAllPlaces() async throws -> AllDataDTO {
    return try await AppNetworkHelper.get(path: APIEndpoints.getAllPlacesUrl)
  }
  
  func getFavorites() async throws -> FavoritesDTO {
    return try await AppNetworkHelper.get(path: APIEndpoints.getFavoritesUrl)
  }
  
  func addFavorites(ids: FavoritesIdsDTO) async throws -> SimpleResponse {
    return try await AppNetworkHelper.post(path: APIEndpoints.addFavoritesUrl, body: ids)
  }
  
  func removeFromFavorites(ids: FavoritesIdsDTO) async throws -> SimpleResponse {
    return try await AppNetworkHelper.delete(path: APIEndpoints.removeFromFavoritesUrl, body: ids)
  }
}
