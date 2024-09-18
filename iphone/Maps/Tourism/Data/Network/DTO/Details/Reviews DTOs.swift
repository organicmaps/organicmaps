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


struct ReviewIdsDTO: Codable {
  let feedbacks: [Int64]
}


struct ReviewsDTO: Codable {
  let data: [ReviewDTO]
}


struct ReviewToPostDTO: Codable {
  let placeId: Int64
  let comment: String
  let rating: Int
  let images: [URL]
}
