import Foundation

struct Review: Codable {
  let id: Int64
  let placeId: Int64
  let rating: Int
  let user: User
  let date: String?
  let comment: String?
  let picsUrls: [String]
  var deletionPlanned: Bool = false
}
