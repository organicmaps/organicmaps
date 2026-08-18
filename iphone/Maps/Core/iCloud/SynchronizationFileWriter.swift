final class SynchronizationFileWriter {
  private let fileManager: FileManager
  // Utility, not background: a background queue is starved while the map is busy, and the user waits for these files.
  private let backgroundQueue = DispatchQueue(label: "iCloud.app.organicmaps.backgroundQueue", qos: .utility)
  private let fileCoordinator: NSFileCoordinator
  private let localDirectoryUrl: URL
  private let cloudDirectoryUrl: URL

  init(fileManager: FileManager = .default,
       fileCoordinator: NSFileCoordinator = NSFileCoordinator(),
       localDirectoryUrl: URL,
       cloudDirectoryUrl: URL) {
    self.fileManager = fileManager
    self.fileCoordinator = fileCoordinator
    self.localDirectoryUrl = localDirectoryUrl
    self.cloudDirectoryUrl = cloudDirectoryUrl
  }

  func processEvent(_ event: OutgoingSynchronizationEvent, completion: @escaping WritingResultCompletionHandler) {
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
                                   completion: resultCompletion)
      case .updateLocalItem(let cloudMetadataItem, let replacedContent, let preservingLocal):
        self.writeToLocalContainer(cloudMetadataItem,
                                   replacing: replacedContent,
                                   preservingLocal: preservingLocal,
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

  /// Writes the cloud file into the local directory, but only while the local file still holds the content this
  /// was decided from -- nothing at all when it was absent then. Anything else there was written after the
  /// decision, by the app itself, and nobody has compared it with the cloud copy yet. `preservingLocal` keeps
  /// that content first, next to the file and under a name derived from it: it is the only copy of it.
  private func writeToLocalContainer(_ cloudMetadataItem: CloudMetadataItem,
                                     replacing expectedContent: Fingerprint?,
                                     preservingLocal: Bool,
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
        if let reason = mismatch(at: writingUrl, expecting: expectedContent, in: "the local directory") {
          completion(.skipped(reason))
          return
        }
        /* The local version is the only copy of itself, and the name it is kept under is derived from what it
         holds -- verified right above. A copy that is there already holds that very content, kept by another
         device or by an earlier conflict of the same file: it is left as it is and reported all the same, so
         that the app loads what is next to the file. */
        var preservedUrl: URL?
        var isCopyWritten = false
        if preservingLocal, let expectedContent {
          preservedUrl = copyUrl(of: writingUrl, holding: expectedContent)
        }
        if let preservedUrl, !fileManager.fileExists(atPath: preservedUrl.path) {
          LOG(.info, "Keep a copy of \(writingUrl.lastPathComponent) as \(preservedUrl.lastPathComponent)")
          try fileManager.copyItem(at: writingUrl, to: preservedUrl)
          isCopyWritten = true
        }
        do {
          try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        } catch {
          // The local file is still there, so a copy written for it preserves nothing and is only a duplicate.
          if isCopyWritten, let preservedUrl {
            try? fileManager.removeItem(at: preservedUrl)
          }
          throw error
        }
        LOG(.debug, "File \(cloudMetadataItem.fileName) is copied to local directory successfully. Start reloading bookmarks...")
        completion(.reloadCategoriesAtURLs([preservedUrl, writingUrl].compactMap { $0 }))
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  /// Reports the file for deletion only while it holds the content that was synchronized -- what it held when
  /// the writer looked, right before the report. A save the app has scheduled on its file thread, or makes
  /// afterwards, is not seen: the app does not coordinate its saves. Such a version travels to the app's trash
  /// with the file, or is written again after the deletion and uploaded as a new one: it is never lost silently.
  /// Deleting is left to the caller: the category has to be unloaded together with its file.
  private func removeFromLocalContainer(_ localMetadataItem: LocalMetadataItem,
                                        _ evidence: DeletionEvidence,
                                        completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Remove file \(localMetadataItem.fileName) from the local directory")
    let targetLocalFileUrl = localMetadataItem.fileUrl
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
        try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        LOG(.debug, "File \(localMetadataItem.fileName) is copied to the cloud directory successfully")
        completion(.success)
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  /// Trashing is irreversible and the decision to do it was made on another queue, so the file is only reported
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

    guard let versionsInConflict = NSFileVersion.unresolvedConflictVersionsOfItem(at: cloudMetadataItem.fileUrl), !versionsInConflict.isEmpty,
          let currentVersion = NSFileVersion.currentVersionOfItem(at: cloudMetadataItem.fileUrl)
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
    var versionsToKeep = [Fingerprint: NSFileVersion]()
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
      guard let unresolvedVersions = NSFileVersion.unresolvedConflictVersionsOfItem(at: currentVersionUrl), !unresolvedVersions.isEmpty else {
        LOG(.info, "File \(cloudMetadataItem.fileName) was already resolved.")
        completion(.success)
        return
      }
      // A version that iCloud added while the contents were being read was never read: removing it now would
      // lose it for good, so the whole conflict is left to a later snapshot.
      guard unresolvedVersions.count == versionsInConflict.count else {
        completion(.skipped("the versions of \(cloudMetadataItem.fileName) in conflict changed while they were read"))
        return
      }
      do {
        for (versionContent, version) in versionsToKeep {
          try self.keep(version, of: currentVersionUrl, holding: versionContent)
        }
        unresolvedVersions.forEach { $0.isResolved = true }
        try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
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

  /// Writes the version next to the file it belongs to, under a name derived from what it holds. A copy that is
  /// there already holds that very content -- another device resolved the same conflict, or an earlier one of
  /// the same file kept the same version -- and is left as it is. The caller holds the write access to the file.
  private func keep(_ version: NSFileVersion, of fileUrl: URL, holding versionContent: Fingerprint) throws {
    let versionCopyUrl = copyUrl(of: fileUrl, holding: versionContent)
    guard !fileManager.fileExists(atPath: versionCopyUrl.path) else {
      LOG(.info, "The version of \(fileUrl.lastPathComponent) is kept as \(versionCopyUrl.lastPathComponent) already")
      return
    }
    var coordinationError: NSError?
    var writingError: Error?
    // The same coordinator: it does not wait for the access it holds on the file this copy is made next to.
    fileCoordinator.coordinate(writingItemAt: versionCopyUrl, error: &coordinationError) { url in
      do {
        try version.replaceItem(at: url)
        LOG(.info, "Keep the version of \(fileUrl.lastPathComponent) as \(url.lastPathComponent)")
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
  }

  // MARK: - Helper methods

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
  /// it the same way, and a file under that name holds this version already.
  func copyUrl(of fileUrl: URL, holding content: Fingerprint) -> URL {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    return fileUrl
      .deletingLastPathComponent()
      .appendingPathComponent("\(baseName)_\(content.fileNameSuffix).\(fileUrl.pathExtension)")
  }
}

// MARK: - FileManager + FileReplacing

private extension FileManager {
  func replaceFileSafe(at targetUrl: URL, with sourceUrl: URL) throws {
    guard fileExists(atPath: targetUrl.path) else {
      LOG(.info, "Target file \(targetUrl.lastPathComponent) doesn't exist. The file will be copied.")
      try copyItem(at: sourceUrl, to: targetUrl)
      return
    }
    let tmpDirectoryUrl = try url(for: .itemReplacementDirectory, in: .userDomainMask, appropriateFor: targetUrl, create: true)
    let tmpUrl = tmpDirectoryUrl.appendingPathComponent(sourceUrl.lastPathComponent)
    try copyItem(at: sourceUrl, to: tmpUrl)
    try replaceItem(at: targetUrl, withItemAt: tmpUrl, backupItemName: nil, options: [.usingNewMetadataOnly], resultingItemURL: nil)
    LOG(.debug, "File \(targetUrl.lastPathComponent) was replaced successfully.")
  }
}
