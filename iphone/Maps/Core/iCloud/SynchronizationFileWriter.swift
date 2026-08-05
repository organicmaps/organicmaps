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
      case .createLocalItem(let cloudMetadataItem): self.createInLocalContainer(cloudMetadataItem, completion: resultCompletion)
      case .updateLocalItem(let cloudMetadataItem, let preservedItem):
        self.updateInLocalContainer(cloudMetadataItem, preserving: preservedItem, completion: resultCompletion)
      case .removeLocalItem(let localMetadataItem, _): self.removeFromLocalContainer(localMetadataItem, completion: resultCompletion)
      case .startDownloading(let cloudMetadataItem): self.startDownloading(cloudMetadataItem, completion: resultCompletion)
      case .createCloudItem(let localMetadataItem): self.createInCloudContainer(localMetadataItem, completion: resultCompletion)
      case .updateCloudItem(let localMetadataItem): self.updateInCloudContainer(localMetadataItem, completion: resultCompletion)
      case .removeCloudItem(let cloudMetadataItem, _): self.removeFromCloudContainer(cloudMetadataItem, completion: resultCompletion)
      case .resolveVersionsConflict(let cloudMetadataItem): self.resolveVersionsConflict(cloudMetadataItem, completion: resultCompletion)
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
       of that is a reason to stop synchronizing: the request is repeated on a later snapshot. */
      LOG(.warning, "Failed to start downloading \(cloudMetadataItem.fileName): \(error.localizedDescription)")
      completion(.failure(SynchronizationError.fileUnavailable))
    }
  }

  /// The file was absent when this was decided. If it is there now it holds something nobody has compared with
  /// the cloud copy yet, and overwriting it would lose it without keeping a copy.
  private func createInLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    let targetLocalFileUrl = cloudMetadataItem.relatedLocalItemUrl(to: localDirectoryUrl)
    guard !fileManager.fileExists(atPath: targetLocalFileUrl.path) else {
      completion(.skipped("\(cloudMetadataItem.fileName) is back in the local directory"))
      return
    }
    writeToLocalContainer(cloudMetadataItem, completion: completion)
  }

  /// The preserved copy is made before the local file is overwritten and the replacement is abandoned when it
  /// fails: that copy is the only one holding the local changes.
  private func updateInLocalContainer(_ cloudMetadataItem: CloudMetadataItem,
                                      preserving localMetadataItem: LocalMetadataItem?,
                                      completion: @escaping WritingResultCompletionHandler) {
    var preservedUrls = [URL]()
    if let localMetadataItem {
      do {
        try preservedUrls.append(preserveCopy(of: localMetadataItem))
      } catch {
        completion(.failure(error))
        return
      }
    }
    let fileManager = fileManager
    writeToLocalContainer(cloudMetadataItem) { result in
      switch result {
      case .reloadCategoriesAtURLs(let urls):
        completion(.reloadCategoriesAtURLs(preservedUrls + urls))
      default:
        // The local file was not replaced after all, so the copy of it preserves nothing and is only a duplicate.
        preservedUrls.forEach { try? fileManager.removeItem(at: $0) }
        completion(result)
      }
    }
  }

  /// The copy gets a name no other device and no earlier conflict can produce, and never replaces an existing
  /// file: every preserved version of every device survives.
  private func preserveCopy(of localMetadataItem: LocalMetadataItem) throws -> URL {
    let fileUrl = localMetadataItem.fileUrl
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    let copyUrl = fileUrl
      .deletingLastPathComponent()
      .appendingPathComponent("\(baseName)_\(UUID().uuidString.prefix(8)).\(fileUrl.pathExtension)")
    LOG(.info, "Keep a copy of \(localMetadataItem.fileName) as \(copyUrl.lastPathComponent) to resolve a conflict")
    try fileManager.copyItem(at: fileUrl, to: copyUrl)
    return copyUrl
  }

  private func writeToLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
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
        try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        LOG(.debug, "File \(cloudMetadataItem.fileName) is copied to local directory successfully. Start reloading bookmarks...")
        completion(.reloadCategoriesAtURLs([writingUrl]))
      } catch {
        completion(.failure(error))
      }
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  private func removeFromLocalContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Remove file \(localMetadataItem.fileName) from the local directory")
    let targetLocalFileUrl = localMetadataItem.fileUrl
    guard fileManager.fileExists(atPath: targetLocalFileUrl.path) else {
      LOG(.warning, "File \(localMetadataItem.fileName) doesn't exist in the local directory and cannot be removed")
      completion(.success)
      return
    }
    completion(.deleteCategoriesAtURLs([targetLocalFileUrl]))
  }

  private func createInCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    guard !fileManager.fileExists(atPath: targetCloudFileUrl.path) else {
      completion(.skipped("\(localMetadataItem.fileName) is back in the cloud directory"))
      return
    }
    writeToCloudContainer(localMetadataItem, completion: completion)
  }

  private func updateInCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    writeToCloudContainer(localMetadataItem, completion: completion)
  }

  private func writeToCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.info, "Write file \(localMetadataItem.fileName) to the cloud directory")
    let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: localMetadataItem.fileUrl, writingItemAt: targetCloudFileUrl, error: &coordinationError) { readingUrl, writingUrl in
      do {
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
  /// as ready to be trashed: the caller authorizes it against the latest observations and calls `trashCloudItems`.
  private func removeFromCloudContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    let targetCloudFileUrl = cloudMetadataItem.fileUrl
    guard fileManager.fileExists(atPath: targetCloudFileUrl.path) else {
      LOG(.warning, "File \(cloudMetadataItem.fileName) doesn't exist in the cloud directory and cannot be moved to the trash")
      completion(.success)
      return
    }
    completion(.trashCloudItemsAtURLs([targetCloudFileUrl]))
  }

  func trashCloudItems(at urls: [URL], completion: @escaping WritingResultCompletionHandler) {
    backgroundQueue.async { [weak self] in
      guard let self else { return }
      do {
        for url in urls {
          LOG(.info, "Trash file \(url.lastPathComponent) to the iCloud trash")
          try fileManager.trashItem(at: url, resultingItemURL: nil)
        }
        DispatchQueue.main.async { completion(.success) }
      } catch {
        DispatchQueue.main.async { completion(.failure(error)) }
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

    let targetCloudFileCopyUrl = generateNewFileUrl(for: cloudMetadataItem.fileUrl)
    var coordinationError: NSError?
    fileCoordinator.coordinate(writingItemAt: currentVersion.url,
                               options: [.forReplacing],
                               writingItemAt: targetCloudFileCopyUrl,
                               options: [],
                               error: &coordinationError) { currentVersionUrl, copyVersionUrl in
      // Check that during the coordination block, the current version of the file have not been already resolved by another process.
      guard let unresolvedVersions = NSFileVersion.unresolvedConflictVersionsOfItem(at: currentVersionUrl), !unresolvedVersions.isEmpty else {
        LOG(.info, "File \(cloudMetadataItem.fileName) was already resolved.")
        completion(.success)
        return
      }
      do {
        // Check if the file was already resolved by another process. The in-memory versions should be marked as resolved.
        guard !fileManager.fileExists(atPath: copyVersionUrl.path) else {
          LOG(.info, "File \(cloudMetadataItem.fileName) was already resolved.")
          try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
          completion(.success)
          return
        }

        LOG(.info, "Duplicate file \(cloudMetadataItem.fileName)...")
        try latestVersionInConflict.replaceItem(at: copyVersionUrl)
        // The modification date should be updated to mark files that was involved into the resolving process.
        try currentVersionUrl.setResourceModificationDate(Date())
        try copyVersionUrl.setResourceModificationDate(Date())
        unresolvedVersions.forEach { $0.isResolved = true }
        try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
        LOG(.info, "File \(cloudMetadataItem.fileName) was successfully resolved.")
        completion(.success)
        return
      } catch {
        completion(.failure(error))
        return
      }
    }

    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  // MARK: - Helper methods

  /// A version kept aside by iCloud's own conflict resolution. The name is derived from the original one, so two
  /// devices resolving the same conflict at the same time produce the same file instead of two copies of it.
  private func generateNewFileUrl(for fileUrl: URL) -> URL {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    return fileUrl.deletingLastPathComponent().appendingPathComponent("\(baseName)_1.\(fileUrl.pathExtension)")
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

// MARK: - URL + ResourceValues

private extension URL {
  func setResourceModificationDate(_ date: Date) throws {
    var url = self
    var resource = try resourceValues(forKeys: [.contentModificationDateKey])
    resource.contentModificationDate = date
    try url.setResourceValues(resource)
  }
}
