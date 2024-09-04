import Foundation

struct CoordinatesDTO: Codable {
  let latitude: String?
  let longitude: String?
  
  func toPlaceLocation(name: String) -> PlaceLocation? {
    guard let latitude = latitude, let longitude = longitude,
          let lat = Double(latitude), let lon = Double(longitude) else {
      return nil
    }
    return PlaceLocation(name: name, lat: lat, lon: lon)
  }
}
