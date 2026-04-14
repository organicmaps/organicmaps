import UniformTypeIdentifiers

extension FileType {
  var fileExtension: String {
    switch self {
    case .kml: return "kml"
    case .kmz: return "kmz"
    case .kmb: return "kmb"
    case .gpx: return "gpx"
    case .geoJson: return "geojson"
    case .json: return "json"
    @unknown default:
      fatalError("Unexpected FileType: \(self)")
    }
  }

  var typeIdentifier: String {
    switch self {
    case .kml: return "com.google.earth.kml"
    case .kmz: return "com.google.earth.kmz"
    case .kmb: return "app.organicmaps.kmb"
    case .gpx: return "com.topografix.gpx"
    case .geoJson: return "public.geojson"
    case .json: return "public.json"
    @unknown default:
      fatalError("Unexpected FileType: \(self)")
    }
  }

  var utType: UTType {
    UTType(filenameExtension: fileExtension)!
  }
}
