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

  var utType: UTType {
    UTType(filenameExtension: fileExtension)!
  }
}
