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
  /// Called on the main queue when a fingerprint that was not known before has been computed.
  var onFingerprintReady: (() -> Void)? { get set }
  /// Returns the fingerprint of the item's content, or nil when it is not known yet. Reading and hashing a file
  /// is too slow for the main queue, so it happens in the background and is reported through `onFingerprintReady`.
  func fingerprint(of item: any MetadataItem) -> Fingerprint?
}

final class FileContentFingerprintProvider: FingerprintProvider {
  private let queue = DispatchQueue(label: "iCloud.app.organicmaps.fingerprints", qos: .utility)
  private let fileCoordinator = NSFileCoordinator()
  private var cache = [String: (identity: FileIdentity, fingerprint: Fingerprint)]()
  private var requestedPaths = Set<String>()

  var onFingerprintReady: (() -> Void)?

  func fingerprint(of item: any MetadataItem) -> Fingerprint? {
    let path = item.fileUrl.path
    if let cached = cache[path], cached.identity == item.identity {
      return cached.fingerprint
    }
    // The same file is observed by a lot of snapshots in a row: read it once.
    guard requestedPaths.insert(path).inserted else { return nil }

    let fileUrl = item.fileUrl
    let identity = item.identity
    queue.async { [weak self] in
      guard let self else { return }
      let fingerprint = read(fileUrl)
      DispatchQueue.main.async {
        self.requestedPaths.remove(path)
        guard let fingerprint else { return }
        self.cache[path] = (identity, fingerprint)
        self.onFingerprintReady?()
      }
    }
    return nil
  }

  /// The read is coordinated: iCloud replaces the files it brings, and the digest of a file caught half-written
  /// would look like a change nobody made and produce a conflict copy of nothing.
  private func read(_ fileUrl: URL) -> Fingerprint? {
    var fingerprint: Fingerprint?
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: fileUrl, error: &coordinationError) { url in
      guard let data = try? Data(contentsOf: url, options: .mappedIfSafe) else {
        LOG(.warning, "Failed to read \(url.lastPathComponent) to compute its fingerprint")
        return
      }
      fingerprint = Fingerprint(hashing: data)
    }
    if let coordinationError {
      LOG(.warning, "Failed to read \(fileUrl.lastPathComponent): \(coordinationError.localizedDescription)")
    }
    return fingerprint
  }
}
