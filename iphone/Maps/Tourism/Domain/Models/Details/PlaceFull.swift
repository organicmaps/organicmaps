import Foundation

struct PlaceFull: Codable {
  let id: Int64
  let name: String
  let rating: Double
  let excerpt: String
  let description: String
  let placeLocation: PlaceLocation?
  let cover: String
  let pics: [String]
  let reviews: [Review]?
  let isFavorite: Bool
  
  func toPlaceShort() -> PlaceShort {
    return PlaceShort(
      id: id,
      name: name,
      cover: cover,
      rating: rating,
      excerpt: excerpt,
      isFavorite: isFavorite
    )
  }
}
