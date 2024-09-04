import Foundation

struct ReviewToPost: Codable {
  let placeId: Int64
  let comment: String
  let rating: Int
  let images: [URL] // Using URL to represent file paths
}
