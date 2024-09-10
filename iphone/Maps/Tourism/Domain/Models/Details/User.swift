import Foundation

struct User: Codable, Hashable {
  let id: Int64
  let name: String
  let pfpUrl: String?
  let countryCodeName: String
}
