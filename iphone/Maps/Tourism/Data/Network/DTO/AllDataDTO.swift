import Foundation

struct AllDataDTO: Codable {
    let attractions: [PlaceDTO]
    let restaurants: [PlaceDTO]
    let accommodations: [PlaceDTO]
    let attractionsHash: String
    let restaurantsHash: String
    let accommodationsHash: String
}
