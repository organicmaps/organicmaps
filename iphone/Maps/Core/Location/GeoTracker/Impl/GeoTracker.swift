class GeoTracker: NSObject, IGeoTracker {
  private let trackerCore: IMWMGeoTrackerCore
  private let locationManager = CLLocationManager()
  private var trackingZones: [String : IMWMGeoTrackerZone]?
  private var zoneTrackers: [String : GeoZoneTracker] = [:]

  init(trackerCore: IMWMGeoTrackerCore) {
    self.trackerCore = trackerCore
    super.init()
    locationManager.delegate = self
  }

  deinit {
    locationManager.delegate = nil
  }

  @objc
  func startTracking() {
    if CLLocationManager.significantLocationChangeMonitoringAvailable() {
      locationManager.startMonitoringSignificantLocationChanges()
    }
  }

  @objc
  func endTracking() {
    locationManager.stopMonitoringSignificantLocationChanges()
  }
}

extension GeoTracker: CLLocationManagerDelegate {
  func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
    if let location = locations.last {
      locationManager.monitoredRegions.forEach {
        locationManager.stopMonitoring(for: $0)
      }

      if CLLocationManager.isMonitoringAvailable(for: CLCircularRegion.self) {
        let zones = trackerCore.geoZones(forLat: location.coordinate.latitude,
                                         lon: location.coordinate.longitude,
                                         accuracy: location.horizontalAccuracy)
        zones.forEach {
          locationManager.startMonitoring(
            for: CLCircularRegion(center: CLLocationCoordinate2DMake($0.latitude,  $0.longitude),
                                  radius: 20,
                                  identifier: $0.identifier))
        }
        trackingZones = zones.reduce(into: [:]) { $0[$1.identifier] = $1 }
      }
    }
  }

  func locationManager(_ manager: CLLocationManager, didEnterRegion region: CLRegion) {
    if let zone = trackingZones?[region.identifier] {
      let zoneTracker = GeoZoneTracker(zone, trackerCore: trackerCore)
      zoneTracker.startTracking()
      zoneTrackers[region.identifier] = zoneTracker
    }
  }

  func locationManager(_ manager: CLLocationManager, didExitRegion region: CLRegion) {
    if let zoneTracker = zoneTrackers[region.identifier] {
      zoneTracker.stopTracking()
      zoneTrackers[region.identifier] = nil
    }
  }
}
