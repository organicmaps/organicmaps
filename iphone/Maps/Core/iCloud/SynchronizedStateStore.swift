/// Content of a file as it was when the local and the cloud directory last held the same one. It is the common
/// base of both sides: without it a file that is missing cannot be told from a file that was deleted, and a
/// local change cannot be told from a remote one.
struct SynchronizedFileState: Codable, Equatable {
  let fingerprint: Fingerprint
  /// How both copies looked at that moment: a copy that still looks the same does not have to be read again.
  var localIdentity: FileIdentity?
  var cloudIdentity: FileIdentity?
}

protocol SynchronizedStateStore: AnyObject {
  func state(for fileName: String) -> SynchronizedFileState?
  func setState(_ state: SynchronizedFileState?, for fileName: String)
  /// Forgets everything when the iCloud account has changed: files of another account share no history. An
  /// account that is momentarily unknown is not another one, so the identity is never optional here.
  func resetIfCloudIdentityChanged(_ identity: Data)
}

/// Keeps the state in a JSON file outside of the synchronized directory, so that it is never synchronized itself.
final class FileSynchronizedStateStore: SynchronizedStateStore {
  private struct Contents: Codable {
    var cloudIdentity: Data?
    var files: [String: SynchronizedFileState]
  }

  private let fileUrl: URL
  private let saveQueue = DispatchQueue(label: "iCloud.app.organicmaps.synchronizedState", qos: .utility)
  private var contents: Contents
  private var isSaveScheduled = false

  init(fileManager: FileManager = .default, fileName: String = "iCloudSynchronizationState.json") {
    let directoryUrl = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
    try? fileManager.createDirectory(at: directoryUrl, withIntermediateDirectories: true)
    fileUrl = directoryUrl.appendingPathComponent(fileName)

    guard let data = try? Data(contentsOf: fileUrl) else {
      contents = Contents(cloudIdentity: nil, files: [:])
      return
    }
    do {
      contents = try JSONDecoder().decode(Contents.self, from: data)
      LOG(.debug, "Loaded the synchronized state of \(contents.files.count) file(s)")
    } catch {
      LOG(.warning, "Failed to read the synchronized state: \(error). All files will be reconciled from scratch.")
      contents = Contents(cloudIdentity: nil, files: [:])
    }
  }

  func state(for fileName: String) -> SynchronizedFileState? {
    contents.files[fileName]
  }

  func setState(_ state: SynchronizedFileState?, for fileName: String) {
    guard contents.files[fileName] != state else { return }
    contents.files[fileName] = state
    scheduleSave()
  }

  func resetIfCloudIdentityChanged(_ identity: Data) {
    guard let storedIdentity = contents.cloudIdentity else {
      contents.cloudIdentity = identity
      scheduleSave()
      return
    }
    guard !Self.isSameIdentity(storedIdentity, identity) else { return }
    LOG(.info, "The iCloud account has changed: the synchronized state is reset")
    contents = Contents(cloudIdentity: identity, files: [:])
    scheduleSave()
  }

  /// A ubiquity identity token is an opaque object that Apple asks to compare with isEqual: two archives of the
  /// same token are not promised to be equal byte for byte.
  private static func isSameIdentity(_ lhs: Data, _ rhs: Data) -> Bool {
    guard let lhsToken = try? NSKeyedUnarchiver.unarchivedObject(ofClasses: [NSObject.self], from: lhs),
          let rhsToken = try? NSKeyedUnarchiver.unarchivedObject(ofClasses: [NSObject.self], from: rhs)
    else { return lhs == rhs }
    return (lhsToken as AnyObject).isEqual(rhsToken)
  }

  /// A lot of files become synchronized in a row: the file is written once, and never on the main queue.
  private func scheduleSave() {
    guard !isSaveScheduled else { return }
    isSaveScheduled = true
    DispatchQueue.main.async { [weak self] in
      guard let self else { return }
      isSaveScheduled = false
      let contents = contents
      let fileUrl = fileUrl
      saveQueue.async {
        do {
          try JSONEncoder().encode(contents).write(to: fileUrl, options: .atomic)
        } catch {
          LOG(.error, "Failed to save the synchronized state: \(error)")
        }
      }
    }
  }
}
