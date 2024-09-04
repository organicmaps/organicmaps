import Foundation

struct CategoryDTO: Codable {
    let data: [PlaceDTO]
    let hash: String
}
