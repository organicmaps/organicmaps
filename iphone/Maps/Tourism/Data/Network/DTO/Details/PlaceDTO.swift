import Foundation

struct PlaceDTO: Codable {
  let id: Int64
  let name: String
  let coordinates: CoordinatesDTO?
  let cover: String
  let feedbacks: [ReviewDTO]?
  let gallery: [String]
  let rating: String
  let shortDescription: String
  let longDescription: String
  
  func toPlaceFull(isFavorite: Bool) -> PlaceFull {
    return PlaceFull(
      id: id,
      name: name,
      rating: Double(rating) ?? 0.0,
      excerpt: shortDescription,
      description: longDescription,
      placeLocation: coordinates?.toPlaceLocation(name: name),
      cover: cover,
      pics: gallery,
      reviews: feedbacks?.map { $0.toReview() } ?? [],
      isFavorite: isFavorite
    )
  }
}
