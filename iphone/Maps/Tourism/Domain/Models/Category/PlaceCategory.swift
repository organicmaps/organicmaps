import Foundation

enum PlaceCategory: String, Codable {
  case sights = "1"
  case restaurants = "2"
  case hotels = "3"
  
  var id: Int64 {
    Int64(self.rawValue)!
  }
  
  var serverName: String {
    switch self {
    case .sights:
      return "attractions"
    case .restaurants:
      return "restaurants"
    case .hotels:
      return "accommodations"
    }
  }
}
