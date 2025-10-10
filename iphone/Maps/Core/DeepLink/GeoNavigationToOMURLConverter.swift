import CoreLocation

@available(iOS 18.4, *)
struct GeoNavigationToOMURLConverter {
  
  enum Action {
    case route(Route)
    case map(Place)
    case search(Place)
  }

  struct Route {
    var source: Place
    var destination: Place
  }

  struct Place {
    var coordinate: CLLocationCoordinate2D?
    var name: String?
  }

  static func convert(_ geoURL: URL) -> URL? {
    guard geoURL.scheme?.lowercased() == "geo-navigation" else {
      LOG(.warning, "Not a geo-navigation URL: \(geoURL.absoluteString)")
      return nil
    }
    guard let components = URLComponents(url: geoURL, resolvingAgainstBaseURL: false),
          let action = components.host?.lowercased() else {
      LOG(.error, "Invalid URL: \(geoURL.absoluteString)")
      return nil
    }

    func parseCoordinates(_ s: String) -> CLLocationCoordinate2D? {
      let parts = s.split(separator: ",").map { $0.trimmingCharacters(in: .whitespaces) }
      guard parts.count == 2, let lat = Double(parts[0]), let lon = Double(parts[1]),
            abs(lat) <= 90, abs(lon) <= 180 else {
        LOG(.warning, "Failed to parse coordinates: \(s)")
        return nil
      }
      return CLLocationCoordinate2D(latitude: lat, longitude: lon)
    }

    func parsePlace(_ s: String?) -> Place? {
      guard let raw = s?.trimmingCharacters(in: .whitespacesAndNewlines),
            !raw.isEmpty,
            let coordinates = parseCoordinates(raw) else {
        LOG(.warning, "Failed to parse place: \(s ?? "nil")")
        return nil
      }
      return Place(coordinate: coordinates, name: nil)
    }

    var omAction: Action? = nil
    switch action {
    case "directions":
      if let source = parsePlace(components.queryValue(for: "source")),
         let destination = parsePlace(components.queryValue(for: "destination")) {
        // TODO: (KK) Implement waypoints parsing when it will be supported by the core url parser.
        omAction = .route(Route(source: source, destination: destination))
      }
    case "place":
      if let coordinates = components.queryValue(for: "coordinate").flatMap(parseCoordinates) {
        omAction = .map(Place(coordinate: coordinates, name: components.queryValue(for: "address")))
      } else if let addr = components.queryValue(for: "address") {
        omAction = .search(Place(coordinate: nil, name: addr))
      }
    default:
      break
    }
    guard let omAction else {
      LOG(.warning, "Unsupported geo-navigation action: \(geoURL.absoluteString)")
      return nil
    }
    return buildOMURL(for: omAction)
  }

  static func buildOMURL(for action: Action) -> URL? {
    switch action {
    case .route(let route):
      return buildRouteURL(route)
    case .map(let place):
      return buildMapURL(place)
    case .search(let place):
      return buildSearchURL(place)
    }
  }

  static private func buildRouteURL(_ route: Route) -> URL? {
    var urlComponents = baseURLComponentsForHost("route")
    var items: [URLQueryItem] = []
    let source = route.source
    if let coordinate = source.coordinate {
      items.append(URLQueryItem(name: "sll", value: "\(coordinate.latitude),\(coordinate.longitude)"))
      let n = source.name?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      items.append(URLQueryItem(name: "saddr", value: n))
    }
    let destination = route.destination
    if let coordinate = destination.coordinate {
      items.append(URLQueryItem(name: "dll", value: "\(coordinate.latitude),\(coordinate.longitude)"))
      let n = destination.name?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      items.append(URLQueryItem(name: "daddr", value: n))
    }
    items.append(URLQueryItem(name: "type", value: MWMRouter.string(from: MWMRouter.type())))
    urlComponents.queryItems = items.isEmpty ? nil : items
    return urlComponents.url
  }

  static private func buildMapURL(_ place: Place) -> URL? {
    var urlComponents = baseURLComponentsForHost("map")
    var items: [URLQueryItem] = [URLQueryItem(name: "v", value: "1")]
    if let coordinates = place.coordinate {
      items.append(URLQueryItem(name: "ll", value: "\(coordinates.latitude),\(coordinates.longitude)"))
    }
    if let name = place.name?.nilIfEmpty {
      items.append(URLQueryItem(name: "n", value: name))
    }
    urlComponents.queryItems = items
    return urlComponents.url
  }

  static private func buildSearchURL(_ place: Place) -> URL? {
    var urlComponents = baseURLComponentsForHost("search")
    var items: [URLQueryItem] = []
    let locale = AppInfo.shared().twoLetterLanguageId
    if !locale.isEmpty {
      items.append(URLQueryItem(name: "locale", value: locale))
    }
    guard let query = place.name?.nilIfEmpty else {
      return nil
    }
    items.append(URLQueryItem(name: "query", value: query))
    urlComponents.queryItems = items.isEmpty ? nil : items
    return urlComponents.url
  }

  static private func baseURLComponentsForHost(_ host: String) -> URLComponents {
    var c = URLComponents()
    c.scheme = "om"
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
