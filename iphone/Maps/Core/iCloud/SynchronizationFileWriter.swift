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

  func processEvent(_ event: OutgoingEvent, completion: @escaping WritingResultCompletionHandler) {
    backgroundQueue.async { [weak self] in
      guard let self else { return }
      switch event {
      case .createLocalItem(let cloudMetadataItem): self.createInLocalContainer(cloudMetadataItem, completion: completion)
      case .updateLocalItem(let cloudMetadataItem): self.updateInLocalContainer(cloudMetadataItem, completion: completion)
      case .removeLocalItem(let cloudMetadataItem): self.removeFromLocalContainer(cloudMetadataItem, completion: completion)
      case .startDownloading(let cloudMetadataItem): self.startDownloading(cloudMetadataItem, completion: completion)
      case .createCloudItem(let localMetadataItem): self.createInCloudContainer(localMetadataItem, completion: completion)
      case .updateCloudItem(let localMetadataItem): self.updateInCloudContainer(localMetadataItem, completion: completion)
      case .removeCloudItem(let localMetadataItem): self.removeFromCloudContainer(localMetadataItem, completion: completion)
      case .resolveVersionsConflict(let cloudMetadataItem): self.resolveVersionsConflict(cloudMetadataItem, completion: completion)
      case .resolveInitialSynchronizationConflict(let localMetadataItem): self.resolveInitialSynchronizationConflict(localMetadataItem, completion: completion)
      case .didFinishInitialSynchronization: completion(.success)
      case .didReceiveError(let error): completion(.failure(error))
      }
    }
  }

  // MARK: - Read/Write/Downloading/Uploading
  private func startDownloading(_ cloudMetadataItem: CloudMetadataItem, completion: WritingResultCompletionHandler) {
    do {
      LOG(.debug, "Start downloading file: \(cloudMetadataItem.fileName)...")
      try fileManager.startDownloadingUbiquitousItem(at: cloudMetadataItem.fileUrl)
      completion(.success)
    } catch {
      completion(.failure(error))
    }
  }

  private func createInLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    let targetLocalFileUrl = cloudMetadataItem.relatedLocalItemUrl(to: localDirectoryUrl)
    guard !fileManager.fileExists(atPath: targetLocalFileUrl.path) else {
      LOG(.debug, "File \(cloudMetadataItem.fileName) already exists in the local iCloud container.")
      completion(.success)
      return
    }
    writeToLocalContainer(cloudMetadataItem, completion: completion)
  }

  private func updateInLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    writeToLocalContainer(cloudMetadataItem, completion: completion)
  }

  private func writeToLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    var coordinationError: NSError?
    let targetLocalFileUrl = cloudMetadataItem.relatedLocalItemUrl(to: localDirectoryUrl)
    LOG(.debug, "File \(cloudMetadataItem.fileName) is downloaded to the local iCloud container. Start coordinating and writing file...")
    fileCoordinator.coordinate(readingItemAt: cloudMetadataItem.fileUrl, writingItemAt: targetLocalFileUrl, error: &coordinationError) { readingUrl, writingUrl in
      do {
        try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        LOG(.debug, "File \(cloudMetadataItem.fileName) is copied to local directory successfully. Start reloading bookmarks...")
        completion(.reloadCategoriesAtURLs([writingUrl]))
      } catch {
        completion(.failure(error))
      }
      return
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  private func removeFromLocalContainer(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.debug, "Start removing file \(cloudMetadataItem.fileName) from the local directory...")
    let targetLocalFileUrl = cloudMetadataItem.relatedLocalItemUrl(to: localDirectoryUrl)
    guard fileManager.fileExists(atPath: targetLocalFileUrl.path) else {
      LOG(.debug, "File \(cloudMetadataItem.fileName) doesn't exist in the local directory and cannot be removed.")
      completion(.success)
      return
    }
    completion(.deleteCategoriesAtURLs([targetLocalFileUrl]))
    LOG(.debug, "File \(cloudMetadataItem.fileName) is removed from the local directory successfully.")
  }

  private func createInCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    guard !fileManager.fileExists(atPath: targetCloudFileUrl.path) else {
      LOG(.debug, "File \(localMetadataItem.fileName) already exists in the cloud directory.")
      completion(.success)
      return
    }
    writeToCloudContainer(localMetadataItem, completion: completion)
  }

  private func updateInCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    writeToCloudContainer(localMetadataItem, completion: completion)
  }

  private func writeToCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.debug, "Start writing file \(localMetadataItem.fileName) to the cloud directory...")
    let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
    var coordinationError: NSError?
    fileCoordinator.coordinate(readingItemAt: localMetadataItem.fileUrl, writingItemAt: targetCloudFileUrl, error: &coordinationError) { readingUrl, writingUrl in
      do {
        try fileManager.replaceFileSafe(at: writingUrl, with: readingUrl)
        LOG(.debug, "File \(localMetadataItem.fileName) is copied to the cloud directory successfully.")
        completion(.success)
      } catch {
        completion(.failure(error))
      }
      return
    }
    if let coordinationError {
      completion(.failure(coordinationError))
    }
  }

  private func removeFromCloudContainer(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.debug, "Start trashing file \(localMetadataItem.fileName)...")
    do {
      let targetCloudFileUrl = localMetadataItem.relatedCloudItemUrl(to: cloudDirectoryUrl)
      try removeDuplicatedFileFromTrashDirectoryIfNeeded(cloudDirectoryUrl: cloudDirectoryUrl, fileName: localMetadataItem.fileName)
      try self.fileManager.trashItem(at: targetCloudFileUrl, resultingItemURL: nil)
      LOG(.debug, "File \(localMetadataItem.fileName) was trashed successfully.")
      completion(.success)
    } catch {
      completion(.failure(error))
    }
  }

  // Remove duplicated file from iCloud's .Trash directory if needed.
  // It's important to avoid the duplicating of names in the trash because we can't control the name of the trashed item.
  private func removeDuplicatedFileFromTrashDirectoryIfNeeded(cloudDirectoryUrl: URL, fileName: String) throws {
    // There are no ways to retrieve the content of iCloud's .Trash directory on macOS.
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      return
    }
    LOG(.debug, "Checking if the file \(fileName) is already in the trash directory...")
    let trashDirectoryUrl = try fileManager.trashDirectoryUrl(for: cloudDirectoryUrl)
    let fileInTrashDirectoryUrl = trashDirectoryUrl.appendingPathComponent(fileName)
    let trashDirectoryContent = try fileManager.contentsOfDirectory(at: trashDirectoryUrl,
                                                                    includingPropertiesForKeys: [],
                                                                    options: [.skipsPackageDescendants, .skipsSubdirectoryDescendants])
    if trashDirectoryContent.contains(fileInTrashDirectoryUrl) {
      LOG(.debug, "File \(fileName) is already in the trash directory. Removing it...")
      try fileManager.removeItem(at: fileInTrashDirectoryUrl)
      LOG(.debug, "File \(fileName) was removed from the trash directory successfully.")
    }
  }

  // MARK: - Merge conflicts resolving
  private func resolveVersionsConflict(_ cloudMetadataItem: CloudMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.debug, "Start resolving version conflict for file \(cloudMetadataItem.fileName)...")

    guard let versionsInConflict = NSFileVersion.unresolvedConflictVersionsOfItem(at: cloudMetadataItem.fileUrl), !versionsInConflict.isEmpty,
          let currentVersion = NSFileVersion.currentVersionOfItem(at: cloudMetadataItem.fileUrl) else {
      LOG(.debug, "No versions in conflict found for file \(cloudMetadataItem.fileName).")
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
      LOG(.debug, "No latest version in conflict found for file \(cloudMetadataItem.fileName).")
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
        LOG(.debug, "File \(cloudMetadataItem.fileName) was already resolved.")
        completion(.success)
        return
      }
      do {
        // Check if the file was already resolved by another process. The in-memory versions should be marked as resolved.
        guard !fileManager.fileExists(atPath: copyVersionUrl.path) else {
          LOG(.debug, "File \(cloudMetadataItem.fileName) was already resolved.")
          try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
          completion(.success)
          return
        }

        LOG(.debug, "Duplicate file \(cloudMetadataItem.fileName)...")
        try latestVersionInConflict.replaceItem(at: copyVersionUrl)
        // The modification date should be updated to mark files that was involved into the resolving process.
        try currentVersionUrl.setResourceModificationDate(Date())
        try copyVersionUrl.setResourceModificationDate(Date())
        unresolvedVersions.forEach { $0.isResolved = true }
        try NSFileVersion.removeOtherVersionsOfItem(at: currentVersionUrl)
        LOG(.debug, "File \(cloudMetadataItem.fileName) was successfully resolved.")
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

  private func resolveInitialSynchronizationConflict(_ localMetadataItem: LocalMetadataItem, completion: @escaping WritingResultCompletionHandler) {
    LOG(.debug, "Start resolving initial sync conflict for file \(localMetadataItem.fileName) by copying with a new name...")
    do {
      let newFileUrl = generateNewFileUrl(for: localMetadataItem.fileUrl, addDeviceName: true)
      try fileManager.copyItem(at: localMetadataItem.fileUrl, to: newFileUrl)
      LOG(.debug, "File \(localMetadataItem.fileName) was successfully resolved.")
      completion(.reloadCategoriesAtURLs([newFileUrl]))
    } catch {
      completion(.failure(error))
    }
  }

  // MARK: - Helper methods
  // Generate a new file URL with a new name for the file with the same name.
  // This method should generate the same name for the same file on different devices during the simultaneous conflict resolving.
  private func generateNewFileUrl(for fileUrl: URL, addDeviceName: Bool = false) -> URL {
    let baseName = fileUrl.deletingPathExtension().lastPathComponent
    let fileExtension = fileUrl.pathExtension
    let newBaseName = baseName + "_1"
    let deviceName = addDeviceName ? "_\(UIDevice.current.name)" : ""
    let newFileName = newBaseName + deviceName + "." + fileExtension
    let newFileUrl = fileUrl.deletingLastPathComponent().appendingPathComponent(newFileName)
    return newFileUrl
  }
}

// MARK: - FileManager + FileReplacing
private extension FileManager {
  func replaceFileSafe(at targetUrl: URL, with sourceUrl: URL) throws {
    guard fileExists(atPath: targetUrl.path) else {
      LOG(.debug, "Source file \(targetUrl.lastPathComponent) doesn't exist. The file will be copied.")
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
    var resource = try resourceValues(forKeys:[.contentModificationDateKey])
    resource.contentModificationDate = date
    try url.setResourceValues(resource)
  }
}
