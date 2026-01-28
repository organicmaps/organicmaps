import CoreLocation

@available(iOS 18.4, *)
struct GeoNavigationToOMURLConverter {

  private enum Scheme {
    static let geoNavigation = "geo-navigation"
    static let om = "om"
  }

  private enum GeoQueryKey {
    static let source = "source"
    static let destination = "destination"
    static let coordinate = "coordinate"
    static let address = "address"
  }

  private enum OMQueryKey {
    static let sourceLatLon = "sll"
    static let sourceAddress = "saddr"
    static let destinationLatLon = "dll"
    static let destinationAddress = "daddr"
    static let routerType = "type"
    static let version = "v"
    static let latLon = "ll"
    static let name = "n"
    static let locale = "locale"
    static let query = "query"
  }

  private enum OMQueryValue {
    static let version1 = "1"
  }

  private enum GeoAction: String {
    case direction = "/directions"
    case place = "/place"
  }

  private enum OMAction {
    case route(Route)
    case map(Place)
    case search(Place)

    var path: String {
      switch self {
      case .route: "route"
      case .map: "map"
      case .search: "search"
      }
    }
  }

  struct Route {
    var source: Place
    var destination: Place
  }

  struct Place {
    var coordinate: CLLocationCoordinate2D?
    var name: String?
  }

  // MARK: - Public

  static func convert(_ geoURL: URL) -> URL? {
    guard geoURL.scheme?.lowercased() == Scheme.geoNavigation else {
      LOG(.warning, "Not a geo-navigation URL: \(geoURL.absoluteString)")
      return nil
    }

    guard let components = URLComponents(url: geoURL, resolvingAgainstBaseURL: false),
          let geoAction = GeoAction(rawValue: components.path.lowercased()) else {
      LOG(.warning, "Invalid URL: \(geoURL.absoluteString)")
      return nil
    }

    func parseCoordinates(_ s: String) -> CLLocationCoordinate2D? {
      let parts = s.split(separator: ",").map { $0.trimmingCharacters(in: .whitespaces) }
      guard parts.count == 2,
            let lat = Double(parts[0]),
            let lon = Double(parts[1]),
            abs(lat) <= 90, abs(lon) <= 180 else {
        LOG(.warning, "Failed to parse coordinates: \(s)")
        return nil
      }
      return CLLocationCoordinate2D(latitude: lat, longitude: lon)
    }

    func parsePlace(_ s: String?) -> Place? {
      let raw = s?.trimmingCharacters(in: .whitespacesAndNewlines)
      guard let raw, !raw.isEmpty else {
        LOG(.warning, "Empty place string")
        return nil
      }
      if let coordinates = parseCoordinates(raw) {
        return Place(coordinate: coordinates, name: nil)
      } else {
        return Place(coordinate: nil, name: s)
      }
    }

    var omAction: OMAction? = nil
    switch geoAction {
    case .direction:
      let source = parsePlace(components.queryValue(for: GeoQueryKey.source))
      let destination = parsePlace(components.queryValue(for: GeoQueryKey.destination))
      if let destination {
        if let source {
          // TODO: (KK) Implement waypoints parsing when it will be supported by the core url parser.
          if source.coordinate != nil && destination.coordinate != nil {
            omAction = .route(Route(source: source, destination: destination))
          } else {
            LOG(.warning, "Source and destination must have coordinates to build a route: \(geoURL.absoluteString)")
          }
        } else {
          if destination.coordinate != nil {
            omAction = .map(destination)
          } else {
            omAction = .search(destination)
          }
        }
      } else {
        LOG(.warning, "Destination is missing: \(geoURL.absoluteString)")
      }
    case .place:
      if let coordinates = components.queryValue(for: GeoQueryKey.coordinate).flatMap(parseCoordinates) {
        omAction = .map(Place(coordinate: coordinates, name: components.queryValue(for: GeoQueryKey.address)))
      } else if let addr = components.queryValue(for: GeoQueryKey.address) {
        omAction = .search(Place(coordinate: nil, name: addr))
      }
    }

    guard let omAction else {
      LOG(.warning, "Unsupported geo-navigation action: \(geoURL.absoluteString)")
      return nil
    }
    return buildOMURL(for: omAction)
  }

  static private func buildOMURL(for action: OMAction) -> URL? {
    switch action {
    case .route(let route):
      return buildRouteURL(route, path: action.path)
    case .map(let place):
      return buildMapURL(place, path: action.path)
    case .search(let place):
      return buildSearchURL(place, path: action.path)
    }
  }

  // MARK: - Builders

  static private func buildRouteURL(_ route: Route, path: String) -> URL? {
    var urlComponents = baseURLComponents(for: path)
    var items: [URLQueryItem] = []

    let source = route.source
    if let coordinate = source.coordinate {
      items.append(URLQueryItem(name: OMQueryKey.sourceLatLon, value: "\(coordinate.latitude),\(coordinate.longitude)"))
      let n = source.name?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      items.append(URLQueryItem(name: OMQueryKey.sourceAddress, value: n))
    }

    let destination = route.destination
    if let coordinate = destination.coordinate {
      items.append(URLQueryItem(name: OMQueryKey.destinationLatLon, value: "\(coordinate.latitude),\(coordinate.longitude)"))
      let n = destination.name?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      items.append(URLQueryItem(name: OMQueryKey.destinationAddress, value: n))
    }

    items.append(URLQueryItem(name: OMQueryKey.routerType, value: MWMRouter.string(from: MWMRouter.type())))
    urlComponents.queryItems = items.isEmpty ? nil : items
    return urlComponents.url
  }

  static private func buildMapURL(_ place: Place, path: String) -> URL? {
    var urlComponents = baseURLComponents(for: path)
    var items: [URLQueryItem] = [URLQueryItem(name: OMQueryKey.version, value: OMQueryValue.version1)]

    if let coordinates = place.coordinate {
      items.append(URLQueryItem(name: OMQueryKey.latLon, value: "\(coordinates.latitude),\(coordinates.longitude)"))
    }
    if let name = place.name?.nilIfEmpty {
      items.append(URLQueryItem(name: OMQueryKey.name, value: name))
    }

    urlComponents.queryItems = items
    return urlComponents.url
  }

  static private func buildSearchURL(_ place: Place, path: String) -> URL? {
    var urlComponents = baseURLComponents(for: path)
    var items: [URLQueryItem] = []

    let locale = AppInfo.shared().twoLetterLanguageId
    if !locale.isEmpty {
      items.append(URLQueryItem(name: OMQueryKey.locale, value: locale))
    }

    guard let query = place.name?.nilIfEmpty else { return nil }
    items.append(URLQueryItem(name: OMQueryKey.query, value: query))

    urlComponents.queryItems = items.isEmpty ? nil : items
    return urlComponents.url
  }

  static private func baseURLComponents(for host: String) -> URLComponents {
    var c = URLComponents()
    c.scheme = Scheme.om
    c.host = host
    return c
  }
}

private extension String {
  var nilIfEmpty: String? { isEmpty ? nil : self }
}

private extension URLComponents {
  func queryValue(for name: String) -> String? {
    queryItems?.first(where: { $0.name == name })?.value
  }
}
