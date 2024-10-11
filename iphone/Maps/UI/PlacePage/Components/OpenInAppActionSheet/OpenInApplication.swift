enum OpenInApplication: Int, CaseIterable {
  case osm
  case googleMaps
  case appleMaps
  case osmAnd
  case yandexMaps
  case dGis
  case cityMapper
  case moovit
  case uber
  case waze
  case goMap
}

extension OpenInApplication {
  static var availableApps: [OpenInApplication] {
    // OSM should always be first in the list.
    let sortedApps: [OpenInApplication] = [.osm] + allCases.filter { $0 != .osm }.sorted(by: { $0.name < $1.name })
    return sortedApps.filter { UIApplication.shared.canOpenURL(URL(string: $0.scheme)!) }
  }

  var name: String {
    switch self {
    case .osm:
      return "OpenStreetMap"
    case .googleMaps:
      return "Google Maps"
    case .appleMaps:
      return "Apple Maps"
    case .osmAnd:
      return "OsmAnd"
    case .yandexMaps:
      return "Yandex Maps"
    case .dGis:
      return "2GIS"
    case .cityMapper:
      return "Citymapper"
    case .moovit:
      return "Moovit"
    case .uber:
      return "Uber"
    case .waze:
      return "Waze"
    case .goMap:
      return "Go Map!!"
    }
  }

  // Schemes should be registered in LSApplicationQueriesSchemes - see Info.plist.
  var scheme: String {
    switch self {
    case .osm:
      return "https://osm.org/go/"
    case .googleMaps:
      return "comgooglemaps://"
    case .appleMaps:
      return "https://maps.apple.com/"
    case .osmAnd:
      return "osmandmaps://"
    case .yandexMaps:
      return "yandexmaps://"
    case .dGis:
      return "dgis://"
    case .cityMapper:
      return "citymapper://"
    case .moovit:
      return "moovit://"
    case .uber:
      return "uber://"
    case .waze:
      return "waze://"
    case .goMap:
      return "gomaposm://"
    }
  }

  func linkWith(coordinates: CLLocationCoordinate2D, zoomLevel: Int = Int(FrameworkHelper.currentZoomLevel()), destinationName: String? = nil) -> String {
    let latitude = String(format: "%.6f", coordinates.latitude)
    let longitude = String(format: "%.6f", coordinates.longitude)
    switch self {
    case .osm:
      return GeoUtil.formattedOsmLink(for: coordinates, zoomLevel: Int32(zoomLevel))
    case .googleMaps:
      return "\(scheme)?&q=\(latitude),\(longitude)&z=\(zoomLevel)"
    case .appleMaps:
      if let destinationName {
        return "\(scheme)?q=\(destinationName)&ll=\(latitude),\(longitude)&z=\(zoomLevel)"
      }
      return "\(scheme)?ll=\(latitude),\(longitude)&z=\(zoomLevel)"
    case .osmAnd:
      if let destinationName {
        return "\(scheme)?lat=\(latitude)&lon=\(longitude)&z=\(zoomLevel)&title=\(destinationName)"
      }
      return "\(scheme)?lat=\(latitude)&lon=\(longitude)&z=\(zoomLevel)"
    case .yandexMaps:
      return "\(scheme)maps.yandex.ru/?pt=\(longitude),\(latitude)&z=\(zoomLevel)"
    case .dGis:
      return "\(scheme)2gis.ru/geo/\(longitude),\(latitude)"
    case .cityMapper:
      return "\(scheme)directions?endcoord=\(latitude),\(longitude)&endname=\(destinationName ?? "")"
    case .moovit:
      if let destinationName {
        return "\(scheme)directions?dest_lat=\(latitude)&dest_lon=\(longitude)&dest_name=\(destinationName)"
      }
      return "\(scheme)directions?dest_lat=\(latitude)&dest_lon=\(longitude)"
    case .uber:
      return "\(scheme)?client_id=&action=setPickup&pickup=my_location&dropoff[latitude]=\(latitude)&dropoff[longitude]=\(longitude)"
    case .waze:
      return "\(scheme)?ll=\(latitude),\(longitude)"
    case .goMap:
      return "\(scheme)edit?center=\(latitude),\(longitude)&zoom=\(zoomLevel)"
    }
  }
}
