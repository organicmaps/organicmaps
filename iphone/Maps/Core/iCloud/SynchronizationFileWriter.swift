final class SynchronizationFileWriter {
  private let fileManager: FileManager
  private let backgroundQueue = DispatchQueue(label: "iCloud.app.organicmaps.backgroundQueue", qos: .background)
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
  /// that content under a new name first: it is the only copy of it.
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
        var preservedUrls = [URL]()
        if preservingLocal {
          try preservedUrls.append(preserveCopy(of: writingUrl))
        }
        do {
          try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        } catch {
          // The local file is still there, so the copy of it preserves nothing and is only a duplicate.
          preservedUrls.forEach { try? fileManager.removeItem(at: $0) }
          throw error
        }
        LOG(.debug, "File \(cloudMetadataItem.fileName) is copied to local directory successfully. Start reloading bookmarks...")
        completion(.reloadCategoriesAtURLs(preservedUrls + [writingUrl]))
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  /// The copy gets a name no other device and no earlier conflict can produce, and never replaces an existing
  /// file: every preserved version of every device survives.
  private func preserveCopy(of fileUrl: URL) throws -> URL {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    let copyUrl = fileUrl
      .deletingLastPathComponent()
      .appendingPathComponent("\(baseName)_\(UUID().uuidString.prefix(8)).\(fileUrl.pathExtension)")
    LOG(.info, "Keep a copy of \(fileUrl.lastPathComponent) as \(copyUrl.lastPathComponent) to resolve a conflict")
    try fileManager.copyItem(at: fileUrl, to: copyUrl)
    return copyUrl
  }

  /// The file must still hold the content the surviving copy was compared with: the app does not coordinate its
  /// own saves, so a version written since is the only copy of itself and nobody has synchronized it yet.
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

  /// The last look at the file before its content is gone, inside the coordinated deletion: another device may
  /// have changed or trashed it while the deletion was crossing the queues.
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

  private func resolveVersionsConflict(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Start resolving version conflict for file \(cloudMetadataItem.fileName)...")

    guard let versionsInConflict = NSFileVersion.unresolvedConflictVersionsOfItem(at: cloudMetadataItem.fileUrl), !versionsInConflict.isEmpty,
          let currentVersion = NSFileVersion.currentVersionOfItem(at: cloudMetadataItem.fileUrl)
    else {
      LOG(.info, "No versions in conflict found for file \(cloudMetadataItem.fileName).")
      completion(.success)
      return
    }

    let sortedVersions = versionsInConflict.sorted { version1, version2 in
      guard let date1 = version1.modificationDate, let date2 = version2.modificationDate else {
        return false
      }
      return date1 > date2
    }

    guard let latestVersionInConflict = sortedVersions.first else {
      LOG(.info, "No latest version in conflict found for file \(cloudMetadataItem.fileName).")
      completion(.success)
      return
    }

    /* What the losing version holds decides where it goes, so both versions are read before the write access is
     taken. Nothing has to be written when the version is preserved already: the file itself holds it -- two
     devices uploaded the same content -- or a copy made by another device does. */
    guard let versionContent = coordinatedFingerprint(of: latestVersionInConflict.url),
          let currentContent = coordinatedFingerprint(of: currentVersion.url)
    else {
      completion(.skipped("the versions of \(cloudMetadataItem.fileName) in conflict cannot be read"))
      return
    }
    let targetCloudFileCopyUrl = versionContent == currentContent
      ? nil
      : copyUrl(for: cloudMetadataItem.fileUrl, keeping: versionContent)

    let resolveVersions: (URL, URL?) -> Void = { currentVersionUrl, copyVersionUrl in
      // Check that during the coordination block, the current version of the file have not been already resolved by another process.
      guard let unresolvedVersions = NSFileVersion.unresolvedConflictVersionsOfItem(at: currentVersionUrl), !unresolvedVersions.isEmpty else {
        LOG(.info, "File \(cloudMetadataItem.fileName) was already resolved.")
        completion(.success)
        return
      }
      do {
        if let copyVersionUrl {
          LOG(.info,
              "Keep the version of \(cloudMetadataItem.fileName) in conflict as \(copyVersionUrl.lastPathComponent)")
          try latestVersionInConflict.replaceItem(at: copyVersionUrl)
        } else {
          LOG(.info, "The version of \(cloudMetadataItem.fileName) in conflict is kept already: nothing to write")
        }
        unresolvedVersions.forEach { $0.isResolved = true }
        try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
        LOG(.info, "File \(cloudMetadataItem.fileName) was successfully resolved.")
        completion(.success)
      } catch {
        completion(.failure(error))
      }
    }

    var coordinationError: NSError?
    if let targetCloudFileCopyUrl {
      fileCoordinator.coordinate(writingItemAt: currentVersion.url,
                                 options: [.forReplacing],
                                 writingItemAt: targetCloudFileCopyUrl,
                                 options: [],
                                 error: &coordinationError) { resolveVersions($0, $1) }
    } else {
      fileCoordinator.coordinate(writingItemAt: currentVersion.url,
                                 options: [.forReplacing],
                                 error: &coordinationError) { resolveVersions($0, nil) }
    }

    if let coordinationError {
      completion(.failure(coordinationError))
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

  /// Where a version kept aside by iCloud's own conflict resolution goes: the first of `<name>_1`, `<name>_2`,
  /// ... that is free, or nil when one of them already holds that version -- the same resolution, made by
  /// another device. A copy holding anything else belongs to another conflict and is left alone. The names are
  /// derived from the original one, so two devices resolving the same conflict produce the same file instead of
  /// two copies of it.
  func copyUrl(for fileUrl: URL, keeping versionContent: Fingerprint) -> URL? {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    let directoryUrl = fileUrl.deletingLastPathComponent()
    var index = 1
    // The directory holds a finite number of files, so a free name is always found.
    while true {
      let copyUrl = directoryUrl.appendingPathComponent("\(baseName)_\(index).\(fileUrl.pathExtension)")
      guard fileManager.fileExists(atPath: copyUrl.path) else { return copyUrl }
      guard coordinatedFingerprint(of: copyUrl) != versionContent else { return nil }
      index += 1
    }
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
