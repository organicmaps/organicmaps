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
}

extension OpenInApplication {

  static var allCases: [OpenInApplication] {
    [
      .osm,
      .googleMaps,
      .appleMaps,
      .osmAnd,
      .yandexMaps,
      .dGis,
      .cityMapper,
      .moovit, 
      .uber,
      .waze
    ]
  }

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
      return "OsmAnd Maps"
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
    }
  }

  func linkForCoordinates(_ coordinates: CLLocationCoordinate2D) -> String {
    let latitude = String(format: "%.6f", coordinates.latitude)
    let longitude = String(format: "%.6f", coordinates.longitude)
    switch self {
    case .osm:
      return GeoUtil.formattedOsmLink(for: coordinates)
    case .googleMaps:
      return "\(scheme)?&q=\(latitude),\(longitude)"
    case .appleMaps:
      return "\(scheme)?q=\(latitude),\(longitude)"
    case .osmAnd:
      return "\(scheme)?lat=\(latitude)&lon=\(longitude)&z=15"
    case .yandexMaps:
      return "\(scheme)maps.yandex.ru/?pt=\(longitude),\(latitude)&z=16"
    case .dGis:
      return "\(scheme)2gis.ru/geo/\(longitude),\(latitude)"
    case .cityMapper:
      return "\(scheme)directions?endcoord=\(latitude),\(longitude)&endname="
    case .moovit:
      return "\(scheme)directions?dest_lat=\(latitude)&dest_lon=\(longitude)"
    case .uber:
      return "\(scheme)?client_id=&action=setPickup&pickup=my_location&dropoff[latitude]=\(latitude)&dropoff[longitude]=\(longitude)"
    case .waze:
      return "\(scheme)?ll=\(latitude),\(longitude)"
    }
  }
}
