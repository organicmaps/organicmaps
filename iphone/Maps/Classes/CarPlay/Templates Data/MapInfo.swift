@available(iOS 12.0, *)
struct MapInfo {
  let type: String
  let trips: [CPTrip]?
  
  init(type: String, trips: [CPTrip]? = nil) {
    self.type = type
    self.trips = trips
  }
}
