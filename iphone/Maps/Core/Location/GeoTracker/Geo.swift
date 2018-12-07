final class Geo: NSObject {
  @objc
  static func geoTracker() -> IGeoTracker {
    let trackerCore = MWMGeoTrackerCore()
    return GeoTracker(trackerCore: trackerCore)
  }
}
