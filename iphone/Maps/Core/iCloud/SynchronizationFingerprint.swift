import CryptoKit

/// Content-based identity of a file: files with equal fingerprints have equal content.
/// Modification dates cannot be used instead: iCloud and the local file system assign them independently.
struct Fingerprint: Hashable, Codable, CustomStringConvertible {
  private let digest: Data

  init(hashing data: Data) {
    digest = Data(SHA256.hash(data: data))
  }

  /// The content of the file, or nil when it cannot be read. The caller must hold the coordinated access to the
  /// file: iCloud replaces the files it brings, and the digest of a file caught half-written would look like a
  /// change nobody made.
  init?(contentsOf fileUrl: URL) {
    guard let data = try? Data(contentsOf: fileUrl, options: .mappedIfSafe) else { return nil }
    self.init(hashing: data)
  }

  /// Names the file that holds this content: 12 hex characters, so that two contents practically never share a
  /// name. A copy kept aside is named after what it holds, so every device that keeps the same version of a
  /// file produces the same copy of it instead of one copy per device.
  var fileNameSuffix: String { Self.hex(digest.prefix(6)) }

  var description: String { Self.hex(digest.prefix(4)) }

  private static func hex(_ bytes: Data) -> String { bytes.map { String(format: "%02x", $0) }.joined() }
}

/// Cheap properties that stand for the file's content: on APFS the modification date has nanosecond precision
/// and every save produces a new one, while a copy keeps the date of the source whose content it holds. Only a
/// same-sized content written at the very same instant keeps the identity. It decides whether the file has to be
/// read again and never which version wins: what is compared is always a fingerprint.
struct FileIdentity: Hashable, Codable {
  let modificationDate: TimeInterval
  let size: Int64
}

protocol FingerprintProvider: AnyObject {
  /// Called on the main queue when what is known about the files has changed: a fingerprint has been computed,
  /// or a file that could not be read is worth asking about again.
  var onContentsMayBeKnown: (() -> Void)? { get set }
  /// Returns the fingerprint of the item's content, or nil when it is not known yet. Reading and hashing a file
  /// is too slow for the main queue, so it happens in the background and is reported through
  /// `onContentsMayBeKnown`.
  func fingerprint(of item: any MetadataItem) -> Fingerprint?
}

final class FileContentFingerprintProvider: FingerprintProvider {
  enum Constants {
    /// How long a file that could not be read is left alone. Nothing reports that it became readable, so it is
    /// read again from time to time, and not on every snapshot: a file iCloud is replacing stays unreadable.
    static let failedReadRetryInterval: TimeInterval = 30
  }

  private let queue = DispatchQueue(label: "iCloud.app.organicmaps.fingerprints", qos: .utility)
  private let fileCoordinator = NSFileCoordinator()
  private let failedReadRetryInterval: TimeInterval
  private var cache = [String: (identity: FileIdentity, fingerprint: Fingerprint)]()
  private var requestedPaths = Set<String>()

  var onContentsMayBeKnown: (() -> Void)?

  init(failedReadRetryInterval: TimeInterval = Constants.failedReadRetryInterval) {
    self.failedReadRetryInterval = failedReadRetryInterval
  }

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
        guard let fingerprint else {
          self.scheduleRetry(of: path)
          return
        }
        self.requestedPaths.remove(path)
        self.cache[path] = (identity, fingerprint)
        self.onContentsMayBeKnown?()
      }
    }
    return nil
  }

  /// The path stays requested until the interval passes, so the file that could not be read is not read again on
  /// every snapshot. Reporting afterwards is what makes it read again: the caller only asks about the files it
  /// still cares about, and nothing else would ever ask about this one.
  private func scheduleRetry(of path: String) {
    DispatchQueue.main.asyncAfter(deadline: .now() + failedReadRetryInterval) { [weak self] in
      guard let self else { return }
      requestedPaths.remove(path)
      onContentsMayBeKnown?()
    }
  }

  /// The read is coordinated: a file being replaced by iCloud is read whole or not at all.
  private func read(_ fileUrl: URL) -> Fingerprint? {
    var fingerprint: Fingerprint?
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: fileUrl, error: &coordinationError) { url in
      fingerprint = Fingerprint(contentsOf: url)
      if fingerprint == nil {
        LOG(.warning, "Failed to read \(url.lastPathComponent) to compute its fingerprint")
      }
    }
    if let coordinationError {
      LOG(.warning, "Failed to read \(fileUrl.lastPathComponent): \(coordinationError.localizedDescription)")
    }
    return fingerprint
  }
}
