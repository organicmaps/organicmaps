import Foundation

@objc(PlaceLocation)
class PlaceLocation: NSObject, Codable {
  @objc let name: String
  @objc let lat: Double
  @objc let lon: Double
  
  init(name: String, lat: Double, lon: Double) {
    self.name = name
    self.lat = lat
    self.lon = lon
  }
  
  func toCoordinatesEntity() -> CoordinatesEntity {
    return CoordinatesEntity(latitude: lat, longitude: lon)
  }
}
