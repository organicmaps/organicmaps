import Foundation

struct Review: Codable, Hashable {
  let id: Int64
  let placeId: Int64
  let rating: Int
  let user: User?
  let date: String?
  let comment: String?
  let picsUrls: [String]
  var deletionPlanned: Bool = false
}


struct ReviewToPost: Codable {
  let placeId: Int64
  let comment: String
  let rating: Int
  let images: [URL]
  
  func toReviewToPostDTO() -> ReviewToPostDTO {
    return ReviewToPostDTO(placeId: placeId, comment: comment, rating: rating, images: images)
  }
}
