import UniformTypeIdentifiers

extension FileType {
  var fileExtension: String {
    switch self {
    case .text: return "kml"
    case .archive: return "kmz"
    case .binary: return "kmb"
    case .gpx: return "gpx"
    case .geoJson: return "gejson"
    case .json: return "json"
    }
  }

  var typeIdentifier: String {
    switch self {
    case .text: return "com.google.earth.kml"
    case .archive: return "com.google.earth.kmz"
    case .binary: return "app.omaps.kmb"
    case .gpx: return "com.topografix.gpx"
    case .geoJson: return "public.geojson"
    case .json: return "public.json"
    }
  }

  @available(iOS 14.0, *)
  var utType: UTType {
    UTType(filenameExtension: fileExtension)!
  }
}

