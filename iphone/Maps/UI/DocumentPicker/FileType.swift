import UniformTypeIdentifiers

// TODO: (KK) Remove this type-wrapper and use custom UTTypeIdentifier that is registered into the Info.plist after updating to the iOS >= 14.0.
struct FileType {
  let fileExtension: String
  let typeIdentifier: String
}

extension FileType {
  static let kml = FileType(fileExtension: "kml", typeIdentifier: "com.google.earth.kml")
  static let kmz = FileType(fileExtension: "kmz", typeIdentifier: "com.google.earth.kmz")
  static let gpx = FileType(fileExtension: "gpx", typeIdentifier: "com.topografix.gpx")
}

// MARK: - FileType + UTType
extension FileType {
  @available(iOS 14.0, *)
  var utType: UTType {
    UTType(filenameExtension: fileExtension)!
  }
}

