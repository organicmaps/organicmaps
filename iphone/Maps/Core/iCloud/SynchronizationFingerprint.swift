import CryptoKit

/// Content-based identity of a file: files with equal fingerprints have equal content.
/// Modification dates cannot be used instead: iCloud and the local file system assign them independently.
struct Fingerprint: Hashable, Codable, CustomStringConvertible {
  private let digest: Data

  init(hashing data: Data) {
    digest = Data(SHA256.hash(data: data))
  }

  var description: String { digest.prefix(4).map { String(format: "%02x", $0) }.joined() }
}

/// Cheap file properties that change when the content changes. They only tell that a file has to be read
/// again and never decide which version of a file wins.
struct FileIdentity: Hashable, Codable {
  let modificationDate: TimeInterval
  let size: Int64
}

protocol FingerprintProvider: AnyObject {
  /// Returns the fingerprint of the item's content or nil when the file cannot be read.
  func fingerprint(of item: any MetadataItem) -> Fingerprint?
}

final class FileContentFingerprintProvider: FingerprintProvider {
  private var cache = [String: (identity: FileIdentity, fingerprint: Fingerprint)]()

  func fingerprint(of item: any MetadataItem) -> Fingerprint? {
    let path = item.fileUrl.path
    if let cached = cache[path], cached.identity == item.identity {
      return cached.fingerprint
    }
    guard let data = try? Data(contentsOf: item.fileUrl, options: .mappedIfSafe) else {
      LOG(.warning, "Failed to read \(item.fileName) to compute its fingerprint")
      return nil
    }
    let fingerprint = Fingerprint(hashing: data)
    cache[path] = (item.identity, fingerprint)
    return fingerprint
  }
}
