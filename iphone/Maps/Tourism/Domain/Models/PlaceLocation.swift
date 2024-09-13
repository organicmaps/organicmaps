import Foundation

struct PlaceLocation: Codable {
  let name: String
  let lat: Double
  let lon: Double
  
  func toCoordinatesEntity() -> CoordinatesEntity {
    return CoordinatesEntity(latitude: lat, longitude: lon)
  }
}
