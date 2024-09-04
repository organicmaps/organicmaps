import Foundation

struct ReviewDTO: Codable {
  let id: Int64
  let markId: Int64
  let images: [String]
  let message: String
  let points: Int
  let createdAt: String
  let user: UserDTO
  
  func toReview() -> Review {
    return Review(
      id: id,
      placeId: markId,
      rating: points,
      user: user.toUser(),
      date: createdAt,
      comment: message,
      picsUrls: images
    )
  }
}
