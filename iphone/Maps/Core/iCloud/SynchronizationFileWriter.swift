/// Immutable bytes retained until Core has applied the reload or preserved this incoming version separately.
final class StagedSynchronizationFile {
  let fileUrl: URL
  let fingerprint: Fingerprint
  private let directoryUrl: URL
  private let fileManager: FileManager
  private var removesStagingDirectory = true

  init(copying source: URL, for destination: URL, fileManager: FileManager) throws {
    self.fileManager = fileManager
    directoryUrl = try fileManager.url(for: .itemReplacementDirectory, in: .userDomainMask,
                                       appropriateFor: destination, create: true)
    fileUrl = directoryUrl.appendingPathComponent(source.lastPathComponent)
    do {
      try fileManager.copyItem(at: source, to: fileUrl)
      fingerprint = try Fingerprint(hashing: Data(contentsOf: fileUrl, options: .mappedIfSafe))
    } catch {
      try? fileManager.removeItem(at: directoryUrl)
      throw error
    }
  }

  deinit {
    if removesStagingDirectory {
      try? fileManager.removeItem(at: directoryUrl)
    }
  }

  func retainOnDisk() {
    removesStagingDirectory = false
  }

  func install(at destination: URL, retainingSource: Bool = false) throws {
    let installedUrl: URL
    if retainingSource {
      installedUrl = directoryUrl.appendingPathComponent(UUID().uuidString).appendingPathExtension("kml")
      try fileManager.copyItem(at: fileUrl, to: installedUrl)
    } else {
      installedUrl = fileUrl
    }
    if fileManager.fileExists(atPath: destination.path) {
      try fileManager.replaceItem(at: destination, withItemAt: installedUrl, backupItemName: nil,
                                  options: [.usingNewMetadataOnly], resultingItemURL: nil)
    } else {
      try fileManager.moveItem(at: installedUrl, to: destination)
    }
  }
}

struct SynchronizationFileVersion {
  let url: URL
  let identifier: AnyObject
  let preserve: (URL) throws -> Void
  let resolve: () -> Void
}

extension SynchronizationFileVersion {
  init(_ version: NSFileVersion) {
    url = version.url
    identifier = version.persistentIdentifier as AnyObject
    preserve = { try version.replaceItem(at: $0) }
    resolve = { version.isResolved = true }
  }
}

/// The three system queries used by conflict resolution, kept together so the complete flow can be exercised.
struct SynchronizationFileVersions {
  let current: (URL) -> SynchronizationFileVersion?
  let unresolved: (URL) -> [SynchronizationFileVersion]
  let removeOtherVersions: (URL) throws -> Void

  static let system = SynchronizationFileVersions(
    current: { NSFileVersion.currentVersionOfItem(at: $0).map(SynchronizationFileVersion.init) },
    unresolved: { (NSFileVersion.unresolvedConflictVersionsOfItem(at: $0) ?? []).map(SynchronizationFileVersion.init) },
    removeOtherVersions: { try NSFileVersion.removeOtherVersionsOfItem(at: $0) }
  )
}

final class SynchronizationFileWriter {
  private let fileManager: FileManager
  // Utility, not background: a background queue is starved while the map is busy, and the user waits for these files.
  private let backgroundQueue = DispatchQueue(label: "iCloud.app.organicmaps.backgroundQueue", qos: .utility)
  private let fileCoordinator: NSFileCoordinator
  private let localDirectoryUrl: URL
  private let cloudDirectoryUrl: URL
  private let versions: SynchronizationFileVersions

  init(fileManager: FileManager = .default,
       fileCoordinator: NSFileCoordinator = NSFileCoordinator(),
       localDirectoryUrl: URL,
       cloudDirectoryUrl: URL,
       versions: SynchronizationFileVersions = .system) {
    self.fileManager = fileManager
    self.fileCoordinator = fileCoordinator
    self.localDirectoryUrl = localDirectoryUrl
    self.cloudDirectoryUrl = cloudDirectoryUrl
    self.versions = versions
  }

  func processEvent(_ event: OutgoingSynchronizationEvent,
                    localFileUpdate: BookmarkFileUpdate? = nil,
                    completion: @escaping WritingResultCompletionHandler) {
    // The result is handled on the main queue, where the synchronization state lives. It must not be delivered
    // synchronously: the file coordination that produced it is still in progress.
    let resultCompletion: WritingResultCompletionHandler = { result in
      DispatchQueue.main.async { completion(result) }
    }
    backgroundQueue.async { [weak self] in
      // Stopping the synchronization releases the writer, and the events still queued on it are dropped with it.
      guard let self else { return }
      switch event {
      case .startDownloading(let cloudMetadataItem):
        self.startDownloading(cloudMetadataItem, completion: resultCompletion)
      case .createLocalItem(let cloudMetadataItem):
        self.writeToLocalContainer(cloudMetadataItem,
                                   replacing: nil,
                                   preservingLocal: false,
                                   localFileUpdate: localFileUpdate,
                                   completion: resultCompletion)
      case .updateLocalItem(let cloudMetadataItem, let replacedContent, let preservingLocal):
        self.writeToLocalContainer(cloudMetadataItem,
                                   replacing: replacedContent,
                                   preservingLocal: preservingLocal,
                                   localFileUpdate: localFileUpdate,
                                   completion: resultCompletion)
      case .removeLocalItem(let localMetadataItem, let evidence):
        self.removeFromLocalContainer(localMetadataItem, evidence, completion: resultCompletion)
      case .createCloudItem(let localMetadataItem):
        self.writeToCloudContainer(localMetadataItem, replacing: nil, completion: resultCompletion)
      case .updateCloudItem(let localMetadataItem, let replacedContent):
        self.writeToCloudContainer(localMetadataItem, replacing: replacedContent, completion: resultCompletion)
      case .removeCloudItem(let cloudMetadataItem, let evidence):
        self.removeFromCloudContainer(cloudMetadataItem, evidence, completion: resultCompletion)
      case .resolveVersionsConflict(let cloudMetadataItem):
        self.resolveVersionsConflict(cloudMetadataItem, completion: resultCompletion)
      }
    }
  }

  // MARK: - Read/Write/Downloading/Uploading

  private func startDownloading(_ cloudMetadataItem: CloudMetadataItem, completion: WritingResultCompletionHandler) {
    LOG(.info, "Start downloading file: \(cloudMetadataItem.fileUrl.path)...")
    do {
      if fileManager.isUbiquitousItem(at: cloudMetadataItem.fileUrl) {
        try fileManager.startDownloadingUbiquitousItem(at: cloudMetadataItem.fileUrl)
      } else {
        LOG(.warning, "File \(cloudMetadataItem.fileUrl.path) is not a ubiquitous item. Skipping download.")
      }
      completion(.success)
    } catch {
      /* Downloading does not start while offline, in Low Data Mode, or when iCloud is busy with the item. None
       of that is worth an error: nothing was written, the request is repeated on a later snapshot, and a
       condition the user has to know about is reported by the item itself in the next one. */
      LOG(.warning, "Failed to start downloading \(cloudMetadataItem.fileName): \(error.localizedDescription)")
      completion(.skipped("downloading \(cloudMetadataItem.fileName) could not be started"))
    }
  }

  /// The Core token serializes the content check and installation with native saves. The immutable source also
  /// survives the later asynchronous reload, whose revision check can reject an intervening in-memory edit.
  private func writeToLocalContainer(_ cloudMetadataItem: CloudMetadataItem,
                                     replacing expectedContent: Fingerprint?,
                                     preservingLocal: Bool,
                                     localFileUpdate: BookmarkFileUpdate?,
                                     completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Write file \(cloudMetadataItem.fileName) to the local directory")
    var coordinationError: NSError?
    let targetLocalFileUrl = cloudMetadataItem.relatedLocalItemUrl(to: localDirectoryUrl)
    fileCoordinator.coordinate(readingItemAt: cloudMetadataItem.fileUrl, writingItemAt: targetLocalFileUrl, error: &coordinationError) { readingUrl, writingUrl in
      do {
        /* The cloud copy this was decided from is gone: iCloud trashed or replaced it between the snapshot and
         this coordinated read. There is nothing to write and nothing is wrong. */
        guard fileManager.fileExists(atPath: readingUrl.path) else {
          completion(.skipped("\(readingUrl.lastPathComponent) is not in iCloud anymore"))
          return
        }
        let source = try StagedSynchronizationFile(copying: readingUrl, for: writingUrl, fileManager: fileManager)
        let install = {
          self.installLocalFile(source, at: writingUrl, replacing: expectedContent,
                                preservingLocal: preservingLocal, completion: completion)
        }
        if let localFileUpdate {
          if !localFileUpdate.performIfUnchanged(install) {
            completion(.skipped("\(cloudMetadataItem.fileName) has newer local edits"))
          }
        } else {
          install()
        }
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  private func installLocalFile(_ source: StagedSynchronizationFile, at writingUrl: URL,
                                replacing expectedContent: Fingerprint?, preservingLocal: Bool,
                                completion: WritingResultCompletionHandler) {
    do {
      if let reason = mismatch(at: writingUrl, expecting: expectedContent, in: "the local directory") {
        completion(.skipped(reason))
        return
      }
      // A conflict copy is an editable category. Its name suggests the content it was created with, but only a
      // coordinated read can prove it still preserves that content.
      var preservedUrl: URL?
      var isCopyWritten = false
      if preservingLocal, let expectedContent {
        let copy = try preserveCopy(of: writingUrl, holding: expectedContent) { url in
          try self.fileManager.copyItem(at: writingUrl, to: url)
        }
        preservedUrl = copy.url
        isCopyWritten = copy.isWritten
      }
      do {
        try source.install(at: writingUrl, retainingSource: true)
      } catch {
        // The local file is still there, so a copy written for it preserves nothing and is only a duplicate.
        if isCopyWritten, let preservedUrl {
          try? fileManager.removeItem(at: preservedUrl)
        }
        throw error
      }
      completion(.reloadCategoriesAtURLs([preservedUrl, writingUrl].compactMap { $0 }, source: source))
    } catch {
      completion(.failure(error))
    }
  }

  /// A racing local edit keeps the original category. Give the incoming bytes a fresh name, which cannot refer
  /// to another loaded category whose queued autosave has not reached disk yet.
  static func preserveIncomingFile(_ source: StagedSynchronizationFile, in directory: URL,
                                   fileManager: FileManager, completion: @escaping (Result<URL, Error>) -> Void) {
    DispatchQueue.global(qos: .utility).async {
      let target = directory.appendingPathComponent("conflict_\(UUID().uuidString).kml")
      let coordinator = NSFileCoordinator()
      var coordinationError: NSError?
      var result: Result<URL, Error>?
      coordinator.coordinate(writingItemAt: target, error: &coordinationError) { url in
        result = Result {
          try fileManager.moveItem(at: source.fileUrl, to: url)
          return url
        }
      }
      let outcome = coordinationError.map { Result<URL, Error>.failure($0) } ?? result!
      if case .failure = outcome {
        source.retainOnDisk()
        LOG(.error, "Incoming bookmark content is retained at \(source.fileUrl.path)")
      }
      DispatchQueue.main.async { completion(outcome) }
    }
  }

  /// Checks the synchronized content before requesting deletion. The manager rechecks the identity and pending
  /// native saves before Core unloads the category and moves its file to the trash.
  private func removeFromLocalContainer(_ localMetadataItem: LocalMetadataItem,
                                        _ evidence: DeletionEvidence,
                                        completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Remove file \(localMetadataItem.fileName) from the local directory")
    let targetLocalFileUrl = localMetadataItem.fileUrl
    let cloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    guard !fileManager.fileExists(atPath: cloudFileUrl.path) else {
      completion(.skipped("\(localMetadataItem.fileName) is back in iCloud"))
      return
    }
    guard fileManager.fileExists(atPath: targetLocalFileUrl.path) else {
      LOG(.warning, "File \(localMetadataItem.fileName) doesn't exist in the local directory and cannot be removed")
      completion(.success)
      return
    }
    guard coordinatedFingerprint(of: targetLocalFileUrl) == evidence.base else {
      completion(.skipped("\(localMetadataItem.fileName) changed since its deletion was decided"))
      return
    }
    completion(.deleteCategory(atURL: targetLocalFileUrl))
  }

  /// Writes the local file into the cloud directory, but only while the cloud file still holds the content this
  /// was decided from -- nothing at all when it was absent then. A version another device uploaded in the
  /// meantime is no conflict for iCloud: it would be overwritten as an ordinary edit and installed over on every
  /// device. A cloud file that cannot be read at all is not the one this was decided for either, and is compared
  /// again once iCloud makes it readable.
  private func writeToCloudContainer(_ localMetadataItem: LocalMetadataItem,
                                     replacing expectedContent: Fingerprint?,
                                     completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Write file \(localMetadataItem.fileName) to the cloud directory")
    let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: localMetadataItem.fileUrl, writingItemAt: targetCloudFileUrl, error: &coordinationError) { readingUrl, writingUrl in
      do {
        // The local file was deleted between the snapshot and this coordinated read: there is nothing to write.
        guard fileManager.fileExists(atPath: readingUrl.path) else {
          completion(.skipped("\(readingUrl.lastPathComponent) is not in the local directory anymore"))
          return
        }
        if let reason = mismatch(at: writingUrl, expecting: expectedContent, in: "iCloud") {
          completion(.skipped(reason))
          return
        }
        let writtenContent = try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        LOG(.debug, "File \(localMetadataItem.fileName) is copied to the cloud directory successfully")
        completion(.written(writtenContent))
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  /// Trashing propagates a deletion across devices and was decided on another queue, so the file is only reported
  /// as ready to be trashed: the caller authorizes it against the latest observations and calls `trashCloudItem`
  /// with the content the file is expected to hold.
  private func removeFromCloudContainer(_ cloudMetadataItem: CloudMetadataItem,
                                        _ evidence: DeletionEvidence,
                                        completion: @escaping WritingResultCompletionHandler) {
    let targetCloudFileUrl = cloudMetadataItem.fileUrl
    guard fileManager.fileExists(atPath: targetCloudFileUrl.path) else {
      LOG(.warning, "File \(cloudMetadataItem.fileName) doesn't exist in the cloud directory and cannot be moved to the trash")
      completion(.success)
      return
    }
    completion(.trashCloudItem(atURL: targetCloudFileUrl, expecting: evidence.base))
  }

  /// The last look at the file before it is moved to the iCloud trash, taken inside the coordinated deletion:
  /// another device may have changed or trashed it while the deletion was crossing the queues. Nothing can slip
  /// in between the look and the deletion -- everything that writes a file in iCloud coordinates its writes.
  func trashCloudItem(at url: URL,
                      expecting expectedContent: Fingerprint,
                      completion: @escaping WritingResultCompletionHandler) {
    backgroundQueue.async { [weak self] in
      guard let self else { return }
      let resultCompletion: WritingResultCompletionHandler = { result in
        DispatchQueue.main.async { completion(result) }
      }
      var coordinationError: NSError?
      fileCoordinator.coordinate(writingItemAt: url, options: [.forDeleting], error: &coordinationError) { url in
        let localFileUrl = self.localDirectoryUrl.appendingPathComponent(url.lastPathComponent)
        guard !self.fileManager.fileExists(atPath: localFileUrl.path) else {
          resultCompletion(.skipped("\(url.lastPathComponent) is back in the local directory"))
          return
        }
        guard self.fileManager.fileExists(atPath: url.path) else {
          resultCompletion(.skipped("\(url.lastPathComponent) is already gone from iCloud"))
          return
        }
        guard Fingerprint(contentsOf: url) == expectedContent else {
          resultCompletion(.skipped("\(url.lastPathComponent) changed in iCloud since its deletion was decided"))
          return
        }
        do {
          LOG(.info, "Trash file \(url.lastPathComponent) to the iCloud trash")
          try self.fileManager.trashItem(at: url, resultingItemURL: nil)
          resultCompletion(.success)
        } catch {
          resultCompletion(.failure(error))
        }
      }
      if let coordinationError {
        resultCompletion(.failure(coordinationError))
      }
    }
  }

  // MARK: - Merge conflicts resolving

  /** Every version iCloud kept aside is kept: the file itself holds one of them, and every distinct content of
   the others is written next to it, under a name derived from that content. The versions are marked resolved
   and removed only once all the copies are in place -- a failure before that leaves the conflict with all its
   versions, and it is resolved again on a later snapshot. */
  private func resolveVersionsConflict(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Start resolving version conflict for file \(cloudMetadataItem.fileName)...")

    let versionsInConflict = versions.unresolved(cloudMetadataItem.fileUrl)
    guard !versionsInConflict.isEmpty, let currentVersion = versions.current(cloudMetadataItem.fileUrl)
    else {
      LOG(.info, "No versions in conflict found for file \(cloudMetadataItem.fileName).")
      completion(.success)
      return
    }

    /* What a version holds decides where it goes, so every one of them is read before the write access is taken;
     a version iCloud has not downloaded is materialized by the coordinated read. The content the file itself
     keeps is not kept a second time, and versions holding the same content are one version of the file. */
    guard let currentContent = coordinatedFingerprint(of: currentVersion.url) else {
      completion(.skipped("the current version of \(cloudMetadataItem.fileName) cannot be read"))
      return
    }
    var versionsToKeep = [Fingerprint: SynchronizationFileVersion]()
    for version in versionsInConflict {
      guard let versionContent = coordinatedFingerprint(of: version.url) else {
        completion(.skipped("a version of \(cloudMetadataItem.fileName) in conflict cannot be read"))
        return
      }
      guard versionContent != currentContent else { continue }
      versionsToKeep[versionContent] = version
    }

    var coordinationError: NSError?
    fileCoordinator.coordinate(writingItemAt: currentVersion.url,
                               options: [.forReplacing],
                               error: &coordinationError) { currentVersionUrl in
      // Another process may have resolved the conflict while this one was waiting for the write access.
      let unresolvedVersions = self.versions.unresolved(currentVersionUrl)
      guard !unresolvedVersions.isEmpty else {
        LOG(.info, "File \(cloudMetadataItem.fileName) was already resolved.")
        completion(.success)
        return
      }
      // A version that iCloud added while the contents were being read was never read: removing it now would
      // lose it for good, so the whole conflict is left to a later snapshot.
      guard unresolvedVersions.count == versionsInConflict.count,
            unresolvedVersions.allSatisfy({ unresolved in
              versionsInConflict.contains { $0.identifier.isEqual(unresolved.identifier) }
            }),
            Fingerprint(contentsOf: currentVersionUrl) == currentContent else {
        completion(.skipped("the versions of \(cloudMetadataItem.fileName) in conflict changed while they were read"))
        return
      }
      do {
        for (versionContent, version) in versionsToKeep {
          try self.keep(version, of: currentVersionUrl, holding: versionContent)
        }
        unresolvedVersions.forEach { $0.resolve() }
        try self.versions.removeOtherVersions(currentVersionUrl)
        LOG(.info, "File \(cloudMetadataItem.fileName) was resolved, versions kept: \(versionsToKeep.count)")
        completion(.success)
      } catch {
        completion(.failure(error))
      }
    }

    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  /// The caller holds the write access to the file whose conflicting versions are being preserved.
  private func keep(_ version: SynchronizationFileVersion, of fileUrl: URL, holding versionContent: Fingerprint) throws {
    _ = try preserveCopy(of: fileUrl, holding: versionContent) { url in
      try version.preserve(url)
    }
  }

  // MARK: - Helper methods

  /// Reuses a copy only when it still holds the preserved version. Edits or name collisions use the first free
  /// numbered name, and checking and writing each candidate share the same coordinated access.
  func preserveCopy(of fileUrl: URL,
                    holding content: Fingerprint,
                    write: (URL) throws -> Void) throws -> (url: URL, isWritten: Bool) {
    let preferredUrl = copyUrl(of: fileUrl, holding: content)
    var candidateUrl = preferredUrl
    var suffix = 0
    while true {
      var coordinationError: NSError?
      var writingError: Error?
      var result: (url: URL, isWritten: Bool)?
      fileCoordinator.coordinate(writingItemAt: candidateUrl, error: &coordinationError) { url in
        if fileManager.fileExists(atPath: url.path) {
          if Fingerprint(contentsOf: url) == content {
            result = (url, false)
          }
          return
        }
        do {
          try write(url)
          LOG(.info, "Keep a version of \(fileUrl.lastPathComponent) as \(url.lastPathComponent)")
          result = (url, true)
        } catch {
          writingError = error
        }
      }
      if let coordinationError {
        throw coordinationError
      }
      if let writingError {
        throw writingError
      }
      if let result {
        return result
      }
      suffix += 1
      candidateUrl = preferredUrl.deletingPathExtension()
        .appendingPathExtension("\(suffix).\(preferredUrl.pathExtension)")
    }
  }

  /// Why the file at the destination is not the one the write was decided for, or nil while it still is: a file
  /// that was absent then must not be there at all -- one that exists but cannot be read holds something nobody
  /// has compared with anything -- and a file that was there must hold exactly the content that was compared.
  /// The caller must hold the write access to the file.
  private func mismatch(at url: URL, expecting expectedContent: Fingerprint?, in place: String) -> String? {
    guard let expectedContent else {
      return fileManager.fileExists(atPath: url.path) ? "\(url.lastPathComponent) is back in \(place)" : nil
    }
    guard Fingerprint(contentsOf: url) != expectedContent else { return nil }
    return "\(url.lastPathComponent) is not the file in \(place) this was decided for"
  }

  /// The content of the file under a coordinated read, or nil when it cannot be read at all. A copy iCloud has
  /// not downloaded yet is materialized by the coordination itself, and a file being replaced is read whole or
  /// not at all.
  private func coordinatedFingerprint(of fileUrl: URL) -> Fingerprint? {
    var fingerprint: Fingerprint?
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: fileUrl, error: &coordinationError) { url in
      fingerprint = Fingerprint(contentsOf: url)
    }
    if let coordinationError {
      LOG(.warning, "Failed to read \(fileUrl.lastPathComponent): \(coordinationError.localizedDescription)")
    }
    return fingerprint
  }

  /// Where a version of the file that is kept aside goes: `<name>_<content>.<extension>`, next to the file it
  /// belongs to. The name is derived from the content alone, so every device that keeps this very version names
  /// it the same way. Existing copies are checked by content before this name can be reused.
  func copyUrl(of fileUrl: URL, holding content: Fingerprint) -> URL {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    return fileUrl
      .deletingLastPathComponent()
      .appendingPathComponent("\(baseName)_\(content.fileNameSuffix).\(fileUrl.pathExtension)")
  }
}

// MARK: - FileManager + FileReplacing

private extension FileManager {
  /// Hash the staged copy: the source can be saved again while a transfer waits on the writer's queue.
  func replaceFileSafe(at targetUrl: URL, with sourceUrl: URL) throws -> Fingerprint {
    let source = try StagedSynchronizationFile(copying: sourceUrl, for: targetUrl, fileManager: self)
    try source.install(at: targetUrl)
    return source.fingerprint
  }
}
