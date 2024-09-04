import Foundation

struct PlaceShort: Codable, Identifiable {
  let id: Int64
  let name: String
  let cover: String?
  let rating: Double?
  let excerpt: String?
  var isFavorite: Bool
}
