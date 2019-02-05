class GeoZoneTracker: NSObject {
  let geoZone: IMWMGeoTrackerZone
  let trackerCore: IMWMGeoTrackerCore
  let locationManager = CLLocationManager()

  init(_ zone: IMWMGeoTrackerZone, trackerCore: IMWMGeoTrackerCore) {
    self.geoZone = zone
    self.trackerCore = trackerCore
    locationManager.desiredAccuracy = kCLLocationAccuracyHundredMeters
    locationManager.distanceFilter = 10
    super.init()
    locationManager.delegate = self
  }

  deinit {
    stopTracking()
    locationManager.delegate = nil
  }

  func startTracking() {
    locationManager.startUpdatingLocation()
  }

  func stopTracking() {
    locationManager.stopUpdatingLocation()
  }
}

extension GeoZoneTracker: CLLocationManagerDelegate {
  func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
    if let visitedLocation = locations.filter({
      $0.horizontalAccuracy <= 30 &&
        $0.distance(from: CLLocation(latitude: geoZone.latitude, longitude: geoZone.longitude)) <= 20
    }).first {
      trackerCore.logEnter(geoZone, location: visitedLocation)
      stopTracking()
    }
  }
}
